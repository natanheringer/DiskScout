#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <time.h>
#include "scanner.h"

// Cache file format version
#define CACHE_VERSION 2
#define CACHE_MAGIC 0x4449534B  // "DISK" in hex

// Cache file structure
typedef struct {
    char path[MAX_PATH_LEN];
    uint64_t size;
    time_t mtime;           // Directory modification time
    uint32_t file_count;
    uint32_t dir_count;
    uint32_t checksum;      // Simple checksum for integrity
} CacheEntry;

typedef struct {
    uint32_t magic;         // Cache file signature
    uint32_t version;       // Cache format version
    uint32_t entry_count;   // Number of cache entries
    uint64_t total_size;    // Total size of all files
    uint32_t file_count;    // Total number of files
    time_t created_at;      // When cache was created
    time_t last_updated;    // When cache was last updated
} CacheHeader;

// Cache management functions
int cache_init(void);
void cache_cleanup(void);
const char* cache_get_path(void);

// Cache operations
int cache_load(const char* scan_path, DirInfo* dirs, int* dir_count, uint64_t* total_size, int* file_count);
int cache_save(const char* scan_path, const DirInfo* dirs, int dir_count, uint64_t total_size, int file_count);
int cache_is_valid(const char* scan_path);
void cache_invalidate(const char* scan_path);

// Utility functions
uint32_t cache_calculate_checksum(const char* path, time_t mtime);
int cache_ensure_directory_exists(void);

#endif
