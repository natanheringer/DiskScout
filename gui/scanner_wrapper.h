#ifndef SCANNER_WRAPPER_H
#define SCANNER_WRAPPER_H

#include <QString>
#include <QStringList>
#include <vector>
#include <cstdint>

// Include C header with extern "C" to avoid conflicts
extern "C" {
    #include "backend_interface.h"
}

// C++ wrapper class for C backend
class ScannerWrapper {
public:
    // Directory information structure for C++
    struct DirectoryInfo {
        QString path;
        uint64_t size;
        int fileCount;
        int dirCount;
        
        DirectoryInfo() : size(0), fileCount(0), dirCount(0) {}
        DirectoryInfo(const QString& p, uint64_t s, int fc, int dc) 
            : path(p), size(s), fileCount(fc), dirCount(dc) {}
    };
    
    // Scan directory and return results
    static bool scanDirectory(const QString& path, 
                             std::vector<DirectoryInfo>& directories,
                             uint64_t& totalSize,
                             int& totalFileCount,
                             int& totalDirCount);
    
    // Load from cache
    static bool loadCache(const QString& path,
                         std::vector<DirectoryInfo>& directories,
                         uint64_t& totalSize,
                         int& totalFileCount,
                         int& totalDirCount);
    
    // Save to cache
    static bool saveCache(const QString& path,
                         const std::vector<DirectoryInfo>& directories,
                         uint64_t totalSize,
                         int totalFileCount,
                         int totalDirCount);
    
    // Check if cache is valid
    static bool isCacheValid(const QString& path);
    
    // Clear cache
    static void clearCache(const QString& path);
    
    // Get scan progress (for progress bar)
    static int getScanProgress();
    static QString getProgressPath();
    
    // Cancel current scan
    static void cancelScan();
    
private:
    // Convert C DirInfo to C++ DirectoryInfo
    static DirectoryInfo convertDirInfo(const DirInfo& dirInfo);
    
    // Convert C++ DirectoryInfo to C DirInfo
    static DirInfo convertToDirInfo(const DirectoryInfo& dirInfo);
};

#endif // SCANNER_WRAPPER_H
