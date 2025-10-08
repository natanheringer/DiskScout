#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "scanner.h"
#include "cache.h"

// external assembly function for fast addition
extern void quick_add(uint64_t *total, uint64_t value);
// Assembly external function
extern int compare_sizes(const void *a, const void *b);
// New assembly optimizations
extern int fast_strcmp_dot(const char *str);
extern int fast_strcmp_dotdot(const char *str);
extern void atomic_inc_file_count(int *file_count);
extern int fast_should_skip(const char *name);
extern void fast_path_copy(char *dest, const char *src, size_t max_len);

// Global arrays
DirInfo dirs[MAX_DIRS];
int dir_count = 0;
int file_count = 0;

// Formats byte size for human readability
void format_size(uint64_t bytes, char *output) {
    if (bytes >= 1099511627776ULL) {
        sprintf(output, "%.2f TB", bytes / 1099511627776.0);
    } else if (bytes >= 1073741824) {
        sprintf(output, "%.2f GB", bytes / 1073741824.0);
    } else if (bytes >= 1048576) {
        sprintf(output, "%.2f MB", bytes / 1048576.0);
    } else if (bytes >= 1024) {
        sprintf(output, "%.2f KB", bytes / 1024.0);
    } else {
        sprintf(output, "%lu B", bytes);
    }
}

// Helper: treat both separators on Windows
static int is_path_separator(char c) {
    return c == '/' || c == '\\';
}

// Returns 1 if `child` is inside `parent` (or equal)
static int is_subpath(const char *parent, const char *child) {
    size_t parent_len = strlen(parent);
    if (strncmp(parent, child, parent_len) != 0) return 0;
    // Exact match or boundary by a separator
    char next = child[parent_len];
    return next == '\0' || is_path_separator(next);
}

// Abbreviate very long paths with a centered ellipsis to fit a column
static void abbreviate_path(const char *input, char *output, size_t max_len) {
    size_t len = strlen(input);
    if (max_len == 0) return;
    if (len <= max_len) {
        // Copy and ensure termination
        strncpy(output, input, max_len);
        output[max_len - 1] = '\0';
        return;
    }
    // Reserve 3 chars for "..."
    if (max_len <= 3) {
        // Fallback, just dots
        memset(output, '.', max_len);
        if (max_len > 0) output[max_len - 1] = '\0';
        return;
    }
    size_t head = (max_len - 3) / 2;
    size_t tail = (max_len - 3) - head;
    // Copy head
    strncpy(output, input, head);
    // Ellipsis
    output[head] = '.'; output[head + 1] = '.'; output[head + 2] = '.';
    // Copy tail
    strncpy(output + head + 3, input + (len - tail), tail);
    // Ensure termination
    output[max_len - 1] = '\0';
}

// Scans and returns main subdirectories
int get_subdirs(const char *path, char subdirs[][MAX_PATH_LEN]) {
    DIR *dir = opendir(path);
    if (!dir) return 0;
    
    int count = 0;
    struct dirent *entry;
    struct stat st;
    char fullpath[MAX_PATH_LEN];
    
    while ((entry = readdir(dir)) != NULL && count < MAX_THREADS) {
        // Ignore . and ..
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Build full path
        snprintf(fullpath, MAX_PATH_LEN, "%s/%s", path, entry->d_name);
        
        if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            strncpy(subdirs[count], fullpath, MAX_PATH_LEN);
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("\nUsage: %s <path>\n", argv[0]);
        printf("Example: %s /home/user\n", argv[0]);
        return 1;
    }
    
    // Initialize cache system
    if (cache_init() != 0) {
        printf("Warning: Failed to initialize cache system\n");
    }
    
    printf("DiskScout v2.0 (Multi-threaded + Cache) - Scanning %s\n", argv[1]);
    printf("\nGouge away the damn bloat outta your disk space!\n");
    printf("Analyzing: %s\n", argv[1]);
    
    // Measures execution time
    clock_t start_time = clock();
    
    uint64_t total = 0;
    int num_subdirs = 0;
    
    // Check cache first
    printf("Checking cache...\n");
    int cache_result = cache_load(argv[1], dirs, &dir_count, &total, &file_count);
    
    if (cache_result == 1) {
        printf("Cache hit! Using cached results.\n");
        printf("Found %d directories and %d files in cache.\n", dir_count, file_count);
    } else if (cache_result == 0) {
        printf("Cache miss or invalid. Performing fresh scan...\n");
    } else {
        printf("Cache error. Performing fresh scan...\n");
    }
    
    // Only perform fresh scan if cache miss
    if (cache_result != 1) {
        // Mutex for thread-safe access
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        
        // Gets main subdirectories
        char subdirs[MAX_THREADS][MAX_PATH_LEN];
        num_subdirs = get_subdirs(argv[1], subdirs);
        
        printf("Found %d top-level directories. Spawning threads...\n", num_subdirs);
        printf("Scanning directories...\n");
        
        // If few subdirs, use single-threaded approach
        if (num_subdirs == 0 || num_subdirs == 1) {
            // Single-threaded (small directory or no subdirs)
            total = scan_directory(argv[1], dirs, &dir_count, &file_count, &mutex);
        } else {
            // Multi-threaded approach
            pthread_t threads[MAX_THREADS];
            ThreadTask tasks[MAX_THREADS];
            int num_threads = num_subdirs < MAX_THREADS ? num_subdirs : MAX_THREADS;
            
            // Creates threads
            for (int i = 0; i < num_threads; i++) {
                strncpy(tasks[i].path, subdirs[i], MAX_PATH_LEN);
                tasks[i].dirs = dirs;                    // Global dirs array (for reference)
                tasks[i].dir_count = &dir_count;         // Global dir_count (for reference)
                tasks[i].file_count = &file_count;       // Shared file counter
                tasks[i].mutex = &mutex;                 // Shared mutex
                tasks[i].total_size = 0;                 // Thread's total size
                tasks[i].local_dir_count = 0;            // Initialize thread-local count
                tasks[i].local_dirs = malloc(MAX_DIRS * sizeof(DirInfo)); // Allocate heap memory
                
                if (!tasks[i].local_dirs) {
                    printf("Error: Failed to allocate memory for thread %d\n", i);
                    exit(1);
                }
                
                pthread_create(&threads[i], NULL, scan_thread_worker, &tasks[i]);
            }
            
            // Waits for threads to finish
            for (int i = 0; i < num_threads; i++) {
                pthread_join(threads[i], NULL);
                total += tasks[i].total_size;
            }
            
            // Merge all thread results into global arrays
            merge_thread_results(tasks, num_threads, dirs, &dir_count);
            
            // Free allocated memory
            for (int i = 0; i < num_threads; i++) {
                free(tasks[i].local_dirs);
            }
            
            // Scans root directory (loose files)
            DIR *root_dir = opendir(argv[1]);
            if (root_dir) {
                struct dirent *entry;
                struct stat st;
                char fullpath[MAX_PATH_LEN];
                
                while ((entry = readdir(root_dir)) != NULL) {
                    // Ignore . and ..
                    if (strcmp(entry->d_name, ".") == 0 || 
                        strcmp(entry->d_name, "..") == 0) {
                        continue;
                    }
                    
                    // Build full path
                    snprintf(fullpath, MAX_PATH_LEN, "%s/%s", argv[1], entry->d_name);
                    
                    if (stat(fullpath, &st) == 0 && S_ISREG(st.st_mode)) {
                        quick_add(&total, st.st_size);
                        
                        // Use atomic assembly function instead of mutex
                        atomic_inc_file_count(&file_count);
                    }
                }
                closedir(root_dir);
            }
        }
        
        // Save results to cache
        printf("Saving results to cache...\n");
        if (cache_save(argv[1], dirs, dir_count, total, file_count) == 0) {
            printf("Cache saved successfully.\n");
        } else {
            printf("Warning: Failed to save cache.\n");
        }
        
        pthread_mutex_destroy(&mutex);
    } else {
        // Cache hit - use cached total
        total = 0;
        for (int i = 0; i < dir_count; i++) {
            total += dirs[i].size;
        }
    }
    
    clock_t end_time = clock();
    double elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("\nProcessing...\n");
    
    // Sort directories by decreasing size
    qsort(dirs, dir_count, sizeof(DirInfo), compare_sizes);

    // Select top 20 non-overlapping directories (avoid listing children of a larger parent)
    DirInfo top_dirs[20];
    int top_count = 0;
    for (int i = 0; i < dir_count && top_count < 20; i++) {
        if (dirs[i].size == 0) continue;
        int is_child = 0;
        for (int k = 0; k < top_count; k++) {
            if (is_subpath(top_dirs[k].path, dirs[i].path)) { is_child = 1; break; }
        }
        if (!is_child) {
            top_dirs[top_count++] = dirs[i];
        }
    }

    // Show top non-overlapping largest directories
    printf("\nTop 20 Largest Directories:\n");
    const int PATH_COL_WIDTH = 70; // visual alignment for long paths
    for (int i = 0; i < top_count; i++) {
        char size_str[32];
        char display_path[PATH_COL_WIDTH + 1];
        format_size(top_dirs[i].size, size_str);
        abbreviate_path(top_dirs[i].path, display_path, sizeof(display_path));
        double percent = (total > 0) ? ((top_dirs[i].size * 100.0) / total) : 0.0;
        printf("%2d. %-70s %10s (%5.1f%%)\n", i + 1, display_path, size_str, percent);
    }
    
    // Final summary
    printf("\n Scan Completed!\n");
    printf("============================================\n");
    char total_str[32];
    format_size(total, total_str);
    printf("Total: %s in %d files and %d directories.\n", 
           total_str, file_count, dir_count);
    printf("Time taken: %.2f seconds.\n", elapsed);
    if (cache_result != 1) {
        printf("Threads used: %d\n", num_subdirs < MAX_THREADS ? num_subdirs : MAX_THREADS);
    } else {
        printf("Cache used: Yes\n");
    }
    
    // Cleanup cache system
    cache_cleanup();
    
    return 0;
}
