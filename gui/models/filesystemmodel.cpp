#include "filesystemmodel.h"
#include "sortproxymodel.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

FileSystemModel::FileSystemModel(QObject *parent)
    : QAbstractItemModel(parent)
    , rootNode(nullptr)
    , totalSize(0)
{
    rootNode = new TreeNode();
}

FileSystemModel::~FileSystemModel()
{
    delete rootNode;
}

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeNode* parentNode = parent.isValid() ? findNode(parent) : rootNode;
    if (!parentNode || row >= static_cast<int>(parentNode->children.size()))
        return QModelIndex();

    TreeNode* childNode = parentNode->children[row];
    return createIndex(row, column, childNode);
}

QModelIndex FileSystemModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    TreeNode* childNode = static_cast<TreeNode*>(child.internalPointer());
    if (!childNode || !childNode->parent || childNode->parent == rootNode)
        return QModelIndex();

    TreeNode* parentNode = childNode->parent;
    return createIndex(parentNode->row, 0, parentNode);
}

int FileSystemModel::rowCount(const QModelIndex &parent) const
{
    TreeNode* parentNode = parent.isValid() ? findNode(parent) : rootNode;
    return parentNode ? static_cast<int>(parentNode->children.size()) : 0;
}

int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 4; // Name, Size, Contents, Modified
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeNode* node = static_cast<TreeNode*>(index.internalPointer());
    if (!node)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: // Name
            return QFileInfo(node->info.path).fileName();
        case 1: // Size
            return formatSize(node->info.size);
        case 2: // Contents
            return QString("%1 items").arg(node->info.dirCount);
        case 3: // Modified
            return QFileInfo(node->info.path).lastModified().toString("yyyy-MM-dd hh:mm");
        }
        break;
    case FileRoles::SizeRole:
        return QVariant::fromValue<qulonglong>(node->info.size);
    case FileRoles::ModifiedRole:
        return modifiedFor(node->info.path);
    case FileRoles::ContentsRole:
        return node->info.dirCount;
        
    case Qt::DecorationRole:
        if (index.column() == 0) {
            return getFileIcon(node->info.path);
        }
        break;
        
    case Qt::BackgroundRole:
        if (index.column() == 1) { // Size column
            return getColor(index);
        }
        break;
        
    case Qt::UserRole:
        return node->info.path;
        
    case Qt::ToolTipRole:
        return QString("Path: %1\nSize: %2\nType: %3")
                .arg(node->info.path)
                .arg(formatSize(node->info.size))
                .arg(QFileInfo(node->info.path).isDir() ? "Directory" : "File");
    }

    return QVariant();
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return "Name";
        case 1: return "Size";
        case 2: return "Contents";
        case 3: return "Modified";
        }
    }
    return QVariant();
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void FileSystemModel::setDirectoryData(const std::vector<ScannerWrapper::DirectoryInfo>& directories, uint64_t totalSize)
{
    beginResetModel();
    
    this->totalSize = totalSize;
    delete rootNode;
    rootNode = new TreeNode();
    
    buildTree(directories);
    
    endResetModel();
}

void FileSystemModel::clear()
{
    beginResetModel();
    delete rootNode;
    rootNode = new TreeNode();
    totalSize = 0;
    endResetModel();
}

QString FileSystemModel::getPath(const QModelIndex& index) const
{
    if (!index.isValid())
        return QString();
        
    TreeNode* node = static_cast<TreeNode*>(index.internalPointer());
    return node ? node->info.path : QString();
}

uint64_t FileSystemModel::getSize(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;
        
    TreeNode* node = static_cast<TreeNode*>(index.internalPointer());
    return node ? node->info.size : 0;
}

QColor FileSystemModel::getColor(const QModelIndex& index) const
{
    if (!index.isValid())
        return QColor();
        
    TreeNode* node = static_cast<TreeNode*>(index.internalPointer());
    if (!node)
        return QColor();
        
    return getFileTypeColor(node->info.path);
}

void FileSystemModel::buildTree(const std::vector<ScannerWrapper::DirectoryInfo>& directories)
{
    // Simple implementation - just add all directories as root children
    // In a real implementation, you'd build a proper tree hierarchy
    
    for (size_t i = 0; i < directories.size(); ++i) {
        TreeNode* node = new TreeNode();
        node->info = directories[i];
        node->parent = rootNode;
        node->row = static_cast<int>(i);
        rootNode->children.push_back(node);
    }
}

FileSystemModel::TreeNode* FileSystemModel::findNode(const QModelIndex& index) const
{
    if (!index.isValid())
        return rootNode;
        
    return static_cast<TreeNode*>(index.internalPointer());
}

QColor FileSystemModel::getFileTypeColor(const QString& path) const
{
    QFileInfo fileInfo(path);
    QString extension = fileInfo.suffix().toLower();
    
    // Color coding based on file type (WinDirStat style)
    if (fileInfo.isDir()) {
        return QColor(100, 100, 100); // Gray for directories
    } else if (extension == "exe" || extension == "dll") {
        return QColor(255, 0, 0); // Red for executables
    } else if (extension == "jpg" || extension == "png" || extension == "gif" || extension == "bmp") {
        return QColor(255, 255, 0); // Yellow for images
    } else if (extension == "mp4" || extension == "avi" || extension == "mkv" || extension == "mov") {
        return QColor(0, 255, 0); // Green for videos
    } else if (extension == "mp3" || extension == "wav" || extension == "flac" || extension == "ogg") {
        return QColor(0, 0, 255); // Blue for audio
    } else if (extension == "pdf" || extension == "doc" || extension == "docx" || extension == "txt") {
        return QColor(255, 0, 255); // Magenta for documents
    } else if (extension == "zip" || extension == "rar" || extension == "7z" || extension == "tar") {
        return QColor(255, 165, 0); // Orange for archives
    } else {
        return QColor(200, 200, 200); // Light gray for others
    }
}

QString FileSystemModel::formatSize(uint64_t bytes) const
{
    if (bytes >= 1099511627776ULL) {
        return QString("%1 TB").arg(bytes / 1099511627776.0, 0, 'f', 2);
    } else if (bytes >= 1073741824) {
        return QString("%1 GB").arg(bytes / 1073741824.0, 0, 'f', 2);
    } else if (bytes >= 1048576) {
        return QString("%1 MB").arg(bytes / 1048576.0, 0, 'f', 2);
    } else if (bytes >= 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
    } else {
        return QString("%1 B").arg(bytes);
    }
}

QIcon FileSystemModel::getFileIcon(const QString& path) const
{
    QFileInfo fileInfo(path);
    
    if (fileInfo.isDir()) {
        return QIcon(":/icons/folder.png");
    } else {
        return QIcon(":/icons/file.png");
    }
}
