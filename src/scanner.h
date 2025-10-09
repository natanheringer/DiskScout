#ifndef SCANNER_H 
#define SCANNER_H 

#include <stdint.h>
#include <pthread.h>

#define MAX_PATH_LEN 4096
#define INITIAL_MAX_DIRS 100000
#define MAX_THREADS 8

// Struct to store directory information 
typedef struct{
    char path[MAX_PATH_LEN];
    uint64_t size;
} DirInfo;

// Struct for thread task
typedef struct {
    char path[MAX_PATH_LEN];
    DirInfo *dirs;                    // Global dirs array (for merging)
    int *dir_count;                   // Global dir_count (for merging)
    int *file_count;                  // Global file_count (shared)
    uint64_t total_size;              // Thread's total size
    pthread_mutex_t *mutex;           // Global mutex
    DirInfo *local_dirs;              // Thread-local directory storage (heap allocated)
    int local_dir_count;              // Thread-local directory count
    int *max_dirs;                    // Pointer to current max_dirs (for dynamic growth)
} ThreadTask;

// Checks if a directory should be skipped
int should_skip(const char *name);

// Scans a directory recursively. Thread-safe.
// Returns total size of directory.
// dirs and dir_count can be NULL if we only want total size
uint64_t scan_directory(
    const char *path,
    DirInfo *dirs,
    int *dir_count,
    int *file_count,
    pthread_mutex_t *mutex,
    int *max_dirs
);

// Worker thread function
void* scan_thread_worker(void *arg);

// Merge thread results into global arrays
void merge_thread_results(ThreadTask *tasks, int num_threads, DirInfo *global_dirs, int *global_dir_count);

// Dynamic array management
int grow_directory_array(DirInfo **dirs, int *max_dirs, int current_count, pthread_mutex_t *mutex);

// Expose the most recent directories buffer pointer in case a realloc moved it
// during scanning. Callers that passed an initial buffer can query this to get
// the final, valid pointer after the scan completes.
DirInfo* scanner_current_dirs(void);

// Live progress helpers
void scanner_progress_set_path(const char* path);
const char* scanner_progress_get_path(void);
void scanner_progress_add_bytes(uint64_t bytes);
uint64_t scanner_progress_get_bytes(void);
void scanner_progress_reset(void);


#endif
