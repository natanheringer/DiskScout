#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>

#define MAX_DIRS 10000
#define MY_MAX_PATH 4096

// struct to store directory information 
typedef struct {
    char path[MY_MAX_PATH];
    uint64_t size;
} DirInfo;

// Assembly functions {these are gonna be implemented into disk_diver.asm}
extern void quick_add(uint64_t *total, uint64_t value);
extern int compare_sizes(const void *e, const void *b);

// global array directories 
DirInfo dirs[MAX_DIRS];
int dir_count = 0; 
int file_count = 0; 

// Recursively scans directories 
uint64_t scan_directory(const char *path) {
    DIR *dir; 
    struct dirent *entry; 
    struct stat st; 
    uint64_t total_size = 0; 
    char fullpath[MY_MAX_PATH];

    dir = opendir(path);
    if(!dir) {
        perror("opendir");
        return 0; 
    }
    while((entry = readdir(dir)) != NULL){
        
         if (file_count % 1000 == 0) {
        printf("\rScanning... %d files found", file_count);
        fflush(stdout);
        }

        // ignore . and ..
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { continue; }

        // build full path 
        snprintf(fullpath, MY_MAX_PATH, "%s/%s", path, entry->d_name);

        if(stat(fullpath, &st) == 0){
            if(S_ISDIR(st.st_mode)) {
            // Is directory: recursively scan 
            uint64_t subdir_size = scan_directory(fullpath);
            quick_add(&total_size, subdir_size);
        
        
                // stores subdir info 
                if (dir_count < MAX_DIRS) {
                    strncpy(dirs[dir_count].path, fullpath, MY_MAX_PATH);
                    dirs[dir_count].size = subdir_size; 
                    dir_count++;
                } 
            } else if (S_ISREG(st.st_mode)) {
                // Is regular file: sum size 
                quick_add(&total_size, st.st_size);
                file_count++;
            }
        } 
    }
    
    closedir(dir);
    return total_size; 
}

// byte size format for human readability
void format_size(uint64_t bytes, char *output) {
    if(bytes >= 1099511627776ULL) {
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


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("\nUsage: %s <path>\n", argv[0]);
        return 1;
    }
    
    printf("DiskScout v1.0 - Scanning %s\n", argv[1]);
    printf("\nGouge away the damn bloat outta your disk space!\n");
    printf("Analyzing: %s\n", argv[1]);

    // measures time execution 
    clock_t start = clock(); 

    // Directory scan
    printf("Scanning directories...\n");
    uint64_t total = scan_directory(argv[1]);

    clock_t ent_time = clock();
    double elapsed = (double)(ent_time - start) / CLOCKS_PER_SEC;

    printf("\nProcessing... \n");

    //sort directories by decrescent size
    qsort(dirs, dir_count, sizeof(DirInfo), compare_sizes);

    // show top 20 largest directories
    printf("\nTop 20 Largest Directories:\n");
    
    int show_count = dir_count < 20 ? dir_count : 20;
    for(int i = 0; i < show_count; i++) {
        char size_str[32];
        format_size(dirs[i].size, size_str);

        double percent = (dirs[i].size * 100.0) / total; 
        printf("%2d. %-50s %10s (%5.1f%%)\n", i + 1, dirs[i].path, size_str, percent);
    }

    // final summary 
    printf("\n Scan Completed!\n");
    char total_str[32];
    format_size(total, total_str);
    printf("Total: %s in %d files and %d directories.\n", total_str, file_count, dir_count);
    printf("Time taken: %.2f seconds.\n", elapsed);
    return 0; 

}