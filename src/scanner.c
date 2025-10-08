#define _FILE_OFFSET_BITS 64   // ensures 64-bit file sizes on Windows/MinGW

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include "scanner.h"

// Assembly function declarations
extern int fast_strcmp_dot(const char *str);
extern int fast_strcmp_dotdot(const char *str);
extern void atomic_inc_file_count(int *file_count);
extern int fast_should_skip(const char *name);

// external assembly functions
extern void quick_add(uint64_t *total, uint64_t value);

// List of directories to ignore 
int should_skip(const char *name) {
    const char *skip_list[] = {
        "node_modes", 
        "node_modules", 
        ".git", ".svn", 
        ".hg", "venv", 
        "__pycache__", 
        ".cache", "Cache"
        , NULL
    };

    for (int i = 0; skip_list[i] != NULL; i++){
        if (strcmp(name, skip_list[i]) == 0) {
            return 1; // should skip
        }
    }
    return 0; // do not skip
}

// main scanning function (thread safe)
uint64_t scan_directory(
    const char *path,
    DirInfo *dirs,
    int *dir_count,
    int *file_count,
    pthread_mutex_t *mutex
) {
    DIR *dir;
    struct dirent *entry; 
    struct stat st; 
    uint64_t total_size = 0;
    char fullpath[MAX_PATH_LEN];

    dir = opendir(path);
    if(!dir) {
        return 0;
    }

    while((entry = readdir(dir)) != NULL){
        // ignore . and .. using fast assembly functions
        if (fast_strcmp_dot(entry->d_name) || fast_strcmp_dotdot(entry->d_name)) {
            continue;
        }

        // ignore problematic directories using fast assembly function
        if (fast_should_skip(entry->d_name)) {
            continue;
        }

        snprintf(fullpath, MAX_PATH_LEN, "%s/%s", path, entry->d_name);

        if (stat(fullpath, &st) == 0){

            if (S_ISDIR(st.st_mode)){
                // is directory: recursively scan
                uint64_t dir_size = scan_directory(fullpath, dirs, dir_count, file_count, mutex);

                // add subdirectory size to current directory total
                total_size += dir_size;

            } else if (S_ISREG(st.st_mode)) {
                // is file = size sum 
                total_size += st.st_size;

                // Use atomic assembly function instead of mutex
                atomic_inc_file_count(file_count);

                // Progress indicator every 1000 files (no mutex needed for read)
                if (*file_count % 1000 == 0) {
                    printf("\rScanning... %d files found", *file_count);
                    fflush(stdout);
                }
            }
        }
    }

    closedir(dir);
    
    // Store the current directory in the results array
    if (dirs && dir_count) {
        pthread_mutex_lock(mutex);
        if (*dir_count < MAX_DIRS) {
            strncpy(dirs[*dir_count].path, path, MAX_PATH_LEN);
            dirs[*dir_count].size = total_size;
            (*dir_count)++;
        }
        pthread_mutex_unlock(mutex);
    }
    
    return total_size;
} 

// worker thread
void* scan_thread_worker(void *arg) {
    
    ThreadTask *task = (ThreadTask *)arg;

    // Initialize thread-local storage
    task->local_dir_count = 0;

    // accumulate total per thread using thread-local storage
    task->total_size = scan_directory(task->path, task->local_dirs, &task->local_dir_count, task->file_count, task->mutex);

    return NULL; 
}

// Merge thread results into global arrays
void merge_thread_results(ThreadTask *tasks, int num_threads, DirInfo *global_dirs, int *global_dir_count) {
    *global_dir_count = 0;
    
    // Merge all thread-local results into global array
    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < tasks[i].local_dir_count && *global_dir_count < MAX_DIRS; j++) {
            global_dirs[*global_dir_count] = tasks[i].local_dirs[j];
            (*global_dir_count)++;
        }
    }
}
