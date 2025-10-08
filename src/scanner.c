#define _FILE_OFFSET_BITS 64   // ensures 64-bit file sizes on Windows/MinGW

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#ifndef FIND_FIRST_EX_LARGE_FETCH
#define FIND_FIRST_EX_LARGE_FETCH 0
#endif
#endif
#include <pthread.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "scanner.h"

// Assembly function declarations
extern int fast_strcmp_dot(const char *str);
extern int fast_strcmp_dotdot(const char *str);
extern void atomic_inc_file_count(int *file_count);
extern int fast_should_skip(const char *name);
extern int fast_wstrcmp_dot(const wchar_t *wstr);
extern int fast_wstrcmp_dotdot(const wchar_t *wstr);
extern int fast_wshould_skip(const wchar_t *wname);

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

#ifdef _WIN32
// Windows-native fast scanner using FindFirstFileExW (UTF-16) with UTF-8 API surface
static uint64_t scan_directory_win(
    const char *path,
    DirInfo *dirs,
    int *dir_count,
    int *file_count,
    pthread_mutex_t *mutex
) {
    wchar_t wpath[MAX_PATH_LEN];
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH_LEN);
    if (wlen <= 0) return 0;

    wchar_t pattern[MAX_PATH_LEN];
    _snwprintf(pattern, MAX_PATH_LEN, L"%ls\\*", wpath);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileExW(
        pattern,
        FindExInfoBasic,
        &ffd,
        FindExSearchNameMatch,
        NULL,
        FIND_FIRST_EX_LARGE_FETCH
    );
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }

    uint64_t total_size = 0;

    do {
        const wchar_t *nameW = ffd.cFileName;
        // skip . and .. using wide-char fast helpers
        if (fast_wstrcmp_dot(nameW) || fast_wstrcmp_dotdot(nameW)) {
            continue;
        }

        // skip via wide-char fast assembly filter (no UTF-8 conversion needed)
        if (fast_wshould_skip(nameW)) {
            continue;
        }

        // Build child wide path and UTF-8 path only once
        wchar_t childW[MAX_PATH_LEN];
        _snwprintf(childW, MAX_PATH_LEN, L"%ls\\%ls", wpath, nameW);

        char childUtf8[MAX_PATH_LEN];
        WideCharToMultiByte(CP_UTF8, 0, childW, -1, childUtf8, MAX_PATH_LEN, NULL, NULL);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Recurse into directory
            uint64_t dir_sz = scan_directory_win(childUtf8, dirs, dir_count, file_count, mutex);
            total_size += dir_sz;
        } else {
            ULARGE_INTEGER sz; sz.LowPart = ffd.nFileSizeLow; sz.HighPart = ffd.nFileSizeHigh;
            total_size += sz.QuadPart;

            atomic_inc_file_count(file_count);
            if (*file_count % 1000 == 0) {
                printf("\rScanning... %d files found", *file_count);
                fflush(stdout);
            }
        }
    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);

    // Store current directory result
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
#endif

// main scanning function (thread safe)
uint64_t scan_directory(
    const char *path,
    DirInfo *dirs,
    int *dir_count,
    int *file_count,
    pthread_mutex_t *mutex
) {
#ifdef _WIN32
    return scan_directory_win(path, dirs, dir_count, file_count, mutex);
#else
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
#endif
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
