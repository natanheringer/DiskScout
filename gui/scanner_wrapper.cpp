#include "scanner_wrapper.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// Include C backend headers
extern "C" {
    #include "backend_interface.h"
}

// Global variables for progress tracking
static int g_scanProgress = 0;
static bool g_cancelScan = false;

bool ScannerWrapper::scanDirectory(const QString& path, 
                                   std::vector<DirectoryInfo>& directories,
                                   uint64_t& totalSize,
                                   int& totalFileCount,
                                   int& totalDirCount) {
    
    // Reset progress
    g_scanProgress = 0;
    g_cancelScan = false;
    
    // Convert QString to C string
    QByteArray pathBytes = path.toUtf8();
    const char* cPath = pathBytes.constData();
    
    // Use backend interface
    DirInfo* dirs = nullptr;
    int dirCount = 0;
    int fileCount = 0;
    
    if (!backend_scan_directory(cPath, &dirs, &dirCount, &totalSize, &fileCount)) {
        qDebug() << "Failed to scan directory";
        return false;
    }
    
    // Convert results to C++ format
    directories.clear();
    directories.reserve(dirCount);
    
    for (int i = 0; i < dirCount; ++i) {
        directories.push_back(convertDirInfo(dirs[i]));
    }
    
    totalFileCount = fileCount;
    totalDirCount = dirCount;
    
    // Cleanup (guard null)
    if (dirs) backend_free_dirs(dirs);
    
    return true;
}

bool ScannerWrapper::loadCache(const QString& path,
                              std::vector<DirectoryInfo>& directories,
                              uint64_t& totalSize,
                              int& totalFileCount,
                              int& totalDirCount) {
    
    // Convert QString to C string
    QByteArray pathBytes = path.toUtf8();
    const char* cPath = pathBytes.constData();
    
    // Use backend interface
    DirInfo* dirs = nullptr;
    int dirCount = 0;
    int fileCount = 0;
    
    if (!backend_load_cache(cPath, &dirs, &dirCount, &totalSize, &fileCount)) {
        qDebug() << "Failed to load cache";
        return false;
    }
    
    // Convert results to C++ format
    directories.clear();
    directories.reserve(dirCount);
    
    for (int i = 0; i < dirCount; ++i) {
        directories.push_back(convertDirInfo(dirs[i]));
    }
    
    totalFileCount = fileCount;
    totalDirCount = dirCount;
    
    // Cleanup (guard null)
    if (dirs) backend_free_dirs(dirs);
    
    return true;
}

bool ScannerWrapper::saveCache(const QString& path,
                              const std::vector<DirectoryInfo>& directories,
                              uint64_t totalSize,
                              int totalFileCount,
                              int totalDirCount) {
    
    // Convert QString to C string
    QByteArray pathBytes = path.toUtf8();
    const char* cPath = pathBytes.constData();
    
    // Convert C++ data to C format
    DirInfo* dirs = (DirInfo*)malloc(directories.size() * sizeof(DirInfo));
    if (!dirs) {
        qDebug() << "Failed to allocate memory for cache save";
        return false;
    }
    
    for (size_t i = 0; i < directories.size(); ++i) {
        dirs[i] = convertToDirInfo(directories[i]);
    }
    
    // Save cache
    const int ok = backend_save_cache(cPath, dirs, directories.size(), totalSize, totalFileCount);
    
    // Cleanup
    free(dirs);
    return ok == 1;
}

bool ScannerWrapper::isCacheValid(const QString& path) {
    // Simple implementation - always return false for now
    Q_UNUSED(path);
    return false;
}

void ScannerWrapper::clearCache(const QString& path) {
    // Simple implementation - do nothing for now
    Q_UNUSED(path);
}

int ScannerWrapper::getScanProgress() {
    return g_scanProgress;
}

void ScannerWrapper::cancelScan() {
    g_cancelScan = true;
}

ScannerWrapper::DirectoryInfo ScannerWrapper::convertDirInfo(const DirInfo& dirInfo) {
    DirectoryInfo result;
    result.path = QString::fromUtf8(dirInfo.path);
    result.size = dirInfo.size;
    // C backend DirInfo only has path + size; file/dir counts are aggregate-level in C
    result.fileCount = 0;
    result.dirCount = 0;
    return result;
}

DirInfo ScannerWrapper::convertToDirInfo(const DirectoryInfo& dirInfo) {
    DirInfo result;
    strncpy(result.path, dirInfo.path.toUtf8().constData(), sizeof(result.path) - 1);
    result.path[sizeof(result.path) - 1] = '\0';
    result.size = dirInfo.size;
    // C backend DirInfo has no per-dir file/dir counts
    return result;
}