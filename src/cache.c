#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "cache.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

// Global cache directory path
static char cache_dir_path[1024] = {0};

// Initialize cache system
int cache_init(void) {
    if (cache_ensure_directory_exists() != 0) {
        return -1;
    }
    
    // Store cache directory path
    const char* cache_dir = cache_get_path();
    strncpy(cache_dir_path, cache_dir, sizeof(cache_dir_path) - 1);
    cache_dir_path[sizeof(cache_dir_path) - 1] = '\0';
    
    return 0;
}

// Generate cache file path for a specific scan path
static void get_cache_file_path(const char* scan_path, char* cache_file_path, size_t max_len) {
    // Create a hash of the scan path for the filename
    uint32_t hash = 0;
    const char* p = scan_path;
    while (*p) {
        hash = hash * 31 + *p;
        p++;
    }
    
    // Convert to positive and create filename
    uint32_t positive_hash = hash & 0x7FFFFFFF;
    snprintf(cache_file_path, max_len, "%s/cache_%08x.db", cache_dir_path, positive_hash);
}

// Get cache directory path
const char* cache_get_path(void) {
    static char cache_dir[1024] = {0};
    
    if (cache_dir[0] == '\0') {
#ifdef _WIN32
        // Windows: Use %APPDATA%/DiskScout
        char appdata_path[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata_path) == S_OK) {
            snprintf(cache_dir, sizeof(cache_dir), "%s/DiskScout", appdata_path);
        } else {
            // Fallback to current directory
            strcpy(cache_dir, "./.diskscout");
        }
#else
        // Linux/Mac: Use ~/.diskscout
        const char* home = getenv("HOME");
        if (home) {
            snprintf(cache_dir, sizeof(cache_dir), "%s/.diskscout", home);
        } else {
            // Fallback to current directory
            strcpy(cache_dir, "./.diskscout");
        }
#endif
    }
    
    return cache_dir;
}

// Ensure cache directory exists
int cache_ensure_directory_exists(void) {
    const char* cache_dir = cache_get_path();
    
#ifdef _WIN32
    // Windows: Create directory recursively
    if (CreateDirectoryA(cache_dir, NULL) == 0) {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS) {
            return -1;
        }
    }
#else
    // Linux/Mac: Create directory with mkdir
    if (mkdir(cache_dir, 0755) != 0) {
        if (errno != EEXIST) {
            return -1;
        }
    }
#endif
    
    return 0;
}

// Calculate simple checksum for cache entry
uint32_t cache_calculate_checksum(const char* path, time_t mtime) {
    uint32_t checksum = 0;
    const char* p = path;
    
    // Simple hash of path
    while (*p) {
        checksum = checksum * 31 + *p;
        p++;
    }
    
    // Include modification time
    checksum ^= (uint32_t)mtime;
    
    return checksum;
}

// Check if cache is valid for given path
int cache_is_valid(const char* scan_path) {
    if (!scan_path || cache_dir_path[0] == '\0') {
        return 0;
    }
    
    char cache_file_path[1024];
    get_cache_file_path(scan_path, cache_file_path, sizeof(cache_file_path));
    
    // Check if cache file exists
    struct stat cache_stat;
    if (stat(cache_file_path, &cache_stat) != 0) {
        return 0; // Cache file doesn't exist
    }
    
    // Check if scan path exists and get its modification time
    struct stat scan_stat;
    if (stat(scan_path, &scan_stat) != 0) {
        return 0; // Scan path doesn't exist
    }
    
    // Cache is valid if it's newer than the scan path
    return cache_stat.st_mtime >= scan_stat.st_mtime;
}

// Load cache for given scan path
int cache_load(const char* scan_path, DirInfo* dirs, int* dir_count, uint64_t* total_size, int* file_count) {
    if (!scan_path || !dirs || !dir_count || !total_size || !file_count) {
        return -1;
    }
    
    if (!cache_is_valid(scan_path)) {
        return 0; // Cache not valid
    }
    
    char cache_file_path[1024];
    get_cache_file_path(scan_path, cache_file_path, sizeof(cache_file_path));
    
#ifdef _WIN32
    HANDLE hFile = CreateFileA(cache_file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap) { CloseHandle(hFile); return -1; }
    void *view = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!view) { CloseHandle(hMap); CloseHandle(hFile); return -1; }
    const unsigned char *p = (const unsigned char *)view;
    const unsigned char *end = p + GetFileSize(hFile, NULL);
    if ((size_t)(end - p) < sizeof(CacheHeader)) { UnmapViewOfFile(view); CloseHandle(hMap); CloseHandle(hFile); return -1; }
    const CacheHeader *headerp = (const CacheHeader *)p; p += sizeof(CacheHeader);
    CacheHeader header = *headerp;
#else
    FILE* file = fopen(cache_file_path, "rb");
    if (!file) {
        return -1;
    }
    CacheHeader header;
    if (fread(&header, sizeof(CacheHeader), 1, file) != 1) {
        fclose(file);
        return -1;
    }
#endif
    
    // Validate cache header
    if (header.magic != CACHE_MAGIC || header.version != CACHE_VERSION) {
#ifdef _WIN32
        UnmapViewOfFile(view);
        CloseHandle(hMap);
        CloseHandle(hFile);
#else
        fclose(file);
#endif
        return -1;
    }
    
    // Read cache entries
    *dir_count = 0;
    *total_size = 0;
    *file_count = 0;
    
    for (uint32_t i = 0; i < header.entry_count && *dir_count < MAX_DIRS; i++) {
        CacheEntry entry;
#ifdef _WIN32
        if ((size_t)(end - p) < sizeof(CacheEntry)) break;
        memcpy(&entry, p, sizeof(CacheEntry));
        p += sizeof(CacheEntry);
#else
        if (fread(&entry, sizeof(CacheEntry), 1, file) != 1) {
            break;
        }
#endif
        
        // Validate checksum
        uint32_t expected_checksum = cache_calculate_checksum(entry.path, entry.mtime);
        if (entry.checksum != expected_checksum) {
            continue; // Skip invalid entry
        }
        
        // Copy to result arrays
        strncpy(dirs[*dir_count].path, entry.path, MAX_PATH_LEN);
        dirs[*dir_count].size = entry.size;
        (*dir_count)++;
        
        *total_size += entry.size;
        *file_count += entry.file_count;
    }
    
#ifdef _WIN32
    UnmapViewOfFile(view);
    CloseHandle(hMap);
    CloseHandle(hFile);
#else
    fclose(file);
#endif
    return 1; // Cache loaded successfully
}

// Save cache for given scan path
int cache_save(const char* scan_path, const DirInfo* dirs, int dir_count, uint64_t total_size, int file_count) {
    if (!scan_path || !dirs || dir_count <= 0) {
        return -1;
    }
    
    char cache_file_path[1024];
    get_cache_file_path(scan_path, cache_file_path, sizeof(cache_file_path));
    
#ifdef _WIN32
    HANDLE hFile = CreateFileA(cache_file_path, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
#else
    FILE* file = fopen(cache_file_path, "wb");
    if (!file) {
        return -1;
    }
#endif
    
    // Get scan path modification time
    struct stat scan_stat;
    time_t scan_mtime = time(NULL);
    if (stat(scan_path, &scan_stat) == 0) {
        scan_mtime = scan_stat.st_mtime;
    }
    
    // Write cache header
    CacheHeader header = {
        .magic = CACHE_MAGIC,
        .version = CACHE_VERSION,
        .entry_count = (uint32_t)dir_count,
        .created_at = time(NULL),
        .last_updated = time(NULL)
    };
    
#ifdef _WIN32
    DWORD totalSize = sizeof(CacheHeader) + (DWORD)(dir_count * sizeof(CacheEntry));
    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, totalSize, NULL);
    if (!hMap) { CloseHandle(hFile); return -1; }
    void *view = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, totalSize);
    if (!view) { CloseHandle(hMap); CloseHandle(hFile); return -1; }
    unsigned char *p = (unsigned char *)view;
    memcpy(p, &header, sizeof(CacheHeader));
    p += sizeof(CacheHeader);
#else
    if (fwrite(&header, sizeof(CacheHeader), 1, file) != 1) {
        fclose(file);
        return -1;
    }
#endif
    
    // Write cache entries
    for (int i = 0; i < dir_count; i++) {
        CacheEntry entry = {
            .size = dirs[i].size,
            .mtime = scan_mtime,
            .file_count = (uint32_t)file_count,
            .dir_count = (uint32_t)dir_count,
            .checksum = cache_calculate_checksum(dirs[i].path, scan_mtime)
        };
        
        strncpy(entry.path, dirs[i].path, MAX_PATH_LEN);
        entry.path[MAX_PATH_LEN - 1] = '\0';
        
    #ifdef _WIN32
        memcpy(p, &entry, sizeof(CacheEntry));
        p += sizeof(CacheEntry);
    #else
        if (fwrite(&entry, sizeof(CacheEntry), 1, file) != 1) {
            fclose(file);
            return -1;
        }
    #endif
    }
    
#ifdef _WIN32
    UnmapViewOfFile(view);
    CloseHandle(hMap);
    CloseHandle(hFile);
    return 0;
#else
    fclose(file);
    return 0;
#endif
}

// Invalidate cache for given path
void cache_invalidate(const char* scan_path) {
    if (scan_path && cache_dir_path[0] != '\0') {
        char cache_file_path[1024];
        get_cache_file_path(scan_path, cache_file_path, sizeof(cache_file_path));
        unlink(cache_file_path);
    }
}

// Cleanup cache system
void cache_cleanup(void) {
    // Nothing to cleanup for now
}
