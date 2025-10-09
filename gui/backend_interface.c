#include "backend_interface.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Include the actual C backend (DirInfo already included via header)
#include "../src/cache.h"

// Global variables for the backend
static DirInfo* g_dirs = NULL;
static int g_max_dirs = 0;
static int g_dir_count = 0;
static int g_file_count = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int backend_init(void) {
    // Initialize cache system
    return cache_init();
}

void backend_cleanup(void) {
    if (g_dirs) { free(g_dirs); g_dirs = NULL; }
    cache_cleanup();
    pthread_mutex_destroy(&g_mutex);
}

int backend_scan_directory(const char* path, 
                          DirInfo** dirs, 
                          int* dir_count, 
                          uint64_t* total_size, 
                          int* total_file_count) {
    
    // Reset global state
    if (g_dirs) {
        free(g_dirs);
        g_dirs = NULL;
    }
    g_max_dirs = 0;
    g_dir_count = 0;
    g_file_count = 0;
    
    // Initialize directory array
    g_max_dirs = INITIAL_MAX_DIRS;
    g_dirs = (DirInfo*)malloc(g_max_dirs * sizeof(DirInfo));
    if (!g_dirs) {
        return 0; // Failed to allocate
    }
    
    // Perform scan
    uint64_t total = scan_directory(path, g_dirs, &g_dir_count, &g_file_count, &g_mutex, &g_max_dirs);
    // If the scanner grew the array, fetch the new pointer
    extern DirInfo* scanner_current_dirs(void);
    DirInfo* current = scanner_current_dirs();
    if (current) g_dirs = current;
    
    // Return results
    *dirs = g_dirs;
    *dir_count = g_dir_count;
    *total_size = total;
    *total_file_count = g_file_count;

    // Basic sanity: if nothing recorded, still return success with zeroes
    // The GUI will show empty results gracefully.
    
    return 1; // Success
}

void backend_free_dirs(DirInfo* dirs) {
    if (!dirs) return;
    if (dirs == g_dirs) {
        free(g_dirs);
        g_dirs = NULL;
        return;
    }
    free(dirs);
}

int backend_load_cache(const char* path, 
                      DirInfo** dirs, 
                      int* dir_count, 
                      uint64_t* total_size, 
                      int* total_file_count) {
    
    // Reset global state
    if (g_dirs) {
        free(g_dirs);
        g_dirs = NULL;
    }
    g_max_dirs = 0;
    g_dir_count = 0;
    g_file_count = 0;
    
    // Initialize directory array for cache loading
    g_max_dirs = INITIAL_MAX_DIRS;
    g_dirs = (DirInfo*)malloc(g_max_dirs * sizeof(DirInfo));
    if (!g_dirs) {
        return 0; // Failed to allocate
    }
    
    // Load cache
    int result = cache_load(path, g_dirs, &g_dir_count, total_size, &g_file_count);
    
    if (result == 1) {
        *dirs = g_dirs;
        *dir_count = g_dir_count;
        *total_file_count = g_file_count;
        return 1; // Success
    }
    
    // Cache load failed
    if (g_dirs) { free(g_dirs); g_dirs = NULL; }
    return 0; // Failed
}

int backend_save_cache(const char* path, 
                      const DirInfo* dirs, 
                      int dir_count, 
                      uint64_t total_size, 
                      int total_file_count) {
    
    // Save cache
    cache_save(path, dirs, dir_count, total_size, total_file_count);
    return 1; // Success
}
