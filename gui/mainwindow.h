#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QStyledItemDelegate>
#include <QStackedWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QThread>
#include <QDateTime>
#include <QFileInfo>
#include <QTimer>
#include <QCloseEvent>
#include <QStorageInfo>
#include <QPointer>
#include <QDialog>
#include <QTextBrowser>
#include <QDialogButtonBox>

#include "scanner_wrapper.h"
#include "models/filesystemmodel.h"
#include "models/sortproxymodel.h"
#include "widgets/sunburstwidget.h"
#include "widgets/treemapwidget.h"

class ScanThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onScanClicked();
    void onBrowseClicked();
    void onPathChanged();
    void onViewModeChanged();
    void onScanCompleted();
    void onScanProgress(int progress);
    void onScanError(const QString& error);
    void onTreeSelectionChanged();
    void onContextMenuRequested(const QPoint& pos);
    void onOpenInExplorer();
    void onDeleteFile();
    void onShowProperties();
    void onShowHelpGuide();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();
    void updateView();
    void showContextMenu(const QPoint& pos);
    QString formatSize(uint64_t bytes);
    void closeEvent(QCloseEvent* event) override;
    void applyLanguage();
    
    // UI Components
    QWidget* centralWidget;
    QSplitter* mainSplitter;
    QTreeView* treeView;
    FileSystemModel* fileSystemModel;
    SortProxyModel* sortProxy;
    QStackedWidget* rightPanel;
    SunburstWidget* sunburstWidget;
    TreemapWidget* treemapWidget;
    
    // Toolbar
    QToolBar* topToolBar;
    QComboBox* pathComboBox;
    QPushButton* browseButton;
    QPushButton* scanButton;
    QPushButton* refreshButton;
    QPushButton* viewModeButton;
    
    // Status bar
    QStatusBar* statusBar;
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QLabel* sizeLabel;
    QLabel* fileCountLabel;
    
    // Data
    std::vector<ScannerWrapper::DirectoryInfo> directories;
    uint64_t totalSize;
    int totalFileCount;
    int totalDirCount;
    QString currentPath;
    
    // Threading
    QPointer<ScanThread> scanThread;
    QTimer* progressTimer;
    
    // Actions
    QAction* scanAction;
    QAction* refreshAction;
    QAction* sunburstAction;
    QAction* treemapAction;
    QAction* openExplorerAction;
    QAction* deleteAction;
    QAction* propertiesAction;
    QAction* howToUseAction;
    // Language menu
    enum AppLanguage { LangEnglish = 0, LangPortuguese = 1, LangSpanish = 2 };
    AppLanguage appLanguage = LangEnglish;
    QAction* langEnglishAction;
    QAction* langPortugueseAction;
    QAction* langSpanishAction;
};

// Delegate to draw the percent bar in the first column
class PercentageBarDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit PercentageBarDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

// Thread for scanning (non-blocking UI)
class ScanThread : public QThread
{
    Q_OBJECT
    
public:
    ScanThread(const QString& path, QObject* parent = nullptr);
    const std::vector<ScannerWrapper::DirectoryInfo>& getDirectories() const { return resultDirectories; }
    uint64_t getTotalSize() const { return resultTotalSize; }
    int getTotalFileCount() const { return resultFileCount; }
    int getTotalDirCount() const { return resultDirCount; }
    
signals:
    void scanCompleted();
    void scanError(const QString& error);
    void progressUpdated(int progress);
    
protected:
    void run() override;
    
private:
    QString scanPath;
    std::vector<ScannerWrapper::DirectoryInfo> resultDirectories;
    uint64_t resultTotalSize = 0;
    int resultFileCount = 0;
    int resultDirCount = 0;
};

#endif // MAINWINDOW_H
