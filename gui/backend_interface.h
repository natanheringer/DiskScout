#ifndef BACKEND_INTERFACE_H
#define BACKEND_INTERFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Use the C backend's DirInfo definition
#include "../src/scanner.h"

// Initialize the backend
int backend_init(void);

// Cleanup the backend
void backend_cleanup(void);

// Scan a directory and return results
int backend_scan_directory(const char* path,
                           DirInfo** dirs,
                           int* dir_count,
                           uint64_t* total_size,
                           int* total_file_count);

// Free directory array
void backend_free_dirs(DirInfo* dirs);

// Load from cache
int backend_load_cache(const char* path,
                       DirInfo** dirs,
                       int* dir_count,
                       uint64_t* total_size,
                       int* total_file_count);

// Save to cache
int backend_save_cache(const char* path,
                       const DirInfo* dirs,
                       int dir_count,
                       uint64_t total_size,
                       int total_file_count);

// Live progress API for GUI polling
int backend_get_progress_percent(void);
const char* backend_get_progress_path(void);
void backend_get_counts(int* files, int* dirs);

#ifdef __cplusplus
}
#endif

#endif // BACKEND_INTERFACE_H
