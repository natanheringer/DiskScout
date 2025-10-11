#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QIcon>
#include <QColor>
#include <QDateTime>
#include <QFileInfo>

#include "../scanner_wrapper.h"

class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FileSystemModel(QObject *parent = nullptr);
    ~FileSystemModel();

    // Role exposed for delegates to fetch percent value
    static constexpr int BarPercentRole = Qt::UserRole + 101; // int 0..100

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Custom methods
    void setDirectoryData(const std::vector<ScannerWrapper::DirectoryInfo>& directories, uint64_t totalSize);
    void clear();
    QString getPath(const QModelIndex& index) const;
    uint64_t getSize(const QModelIndex& index) const;
    QColor getColor(const QModelIndex& index) const;

private:

    struct TreeNode {
        ScannerWrapper::DirectoryInfo info;
        std::vector<TreeNode*> children;
        TreeNode* parent;
        int row;
        
        TreeNode() : parent(nullptr), row(0) {}
        ~TreeNode() {
            for (auto child : children) {
                delete child;
            }
        }
    };
    
    TreeNode* rootNode;
    uint64_t totalSize;
    
    void buildTree(const std::vector<ScannerWrapper::DirectoryInfo>& directories);
    TreeNode* findNode(const QModelIndex& index) const;
    QColor getFileTypeColor(const QString& path) const;
    QString formatSize(uint64_t bytes) const;
    QDateTime modifiedFor(const QString& path) const { return QFileInfo(path).lastModified(); }
    QIcon getFileIcon(const QString& path) const;
};

#endif // FILESYSTEMMODEL_H
