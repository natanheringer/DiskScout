#include "mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QPainter>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>
#include "backend_interface.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget(nullptr)
    , mainSplitter(nullptr)
    , treeView(nullptr)
    , rightPanel(nullptr)
    , sunburstWidget(nullptr)
    , treemapWidget(nullptr)
    , topToolBar(nullptr)
    , pathComboBox(nullptr)
    , scanButton(nullptr)
    , refreshButton(nullptr)
    , viewModeButton(nullptr)
    , statusBar(nullptr)
    , progressBar(nullptr)
    , statusLabel(nullptr)
    , sizeLabel(nullptr)
    , fileCountLabel(nullptr)
    , totalSize(0)
    , totalFileCount(0)
    , totalDirCount(0)
    , scanThread(nullptr)
    , progressTimer(nullptr)
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupConnections();
    
    // Set window properties
    setWindowTitle("DiskScout GUI v2.0");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // Load settings
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    // Initialize with default path but do not force C:\; use first available
    currentPath = pathComboBox->count() > 0 ? pathComboBox->itemText(0) : QDir::rootPath();
    pathComboBox->setCurrentText(currentPath);
}

MainWindow::~MainWindow()
{
    // Save settings
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    if (scanThread && scanThread->isRunning()) {
        scanThread->terminate();
        scanThread->wait();
    }
}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main splitter
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Create tree view (left panel)
    treeView = new QTreeView(this);
    fileSystemModel = new FileSystemModel(this);
    sortProxy = new SortProxyModel(this);
    sortProxy->setSourceModel(fileSystemModel);
    treeView->setModel(sortProxy);
    treeView->setItemDelegateForColumn(0, new PercentageBarDelegate(treeView));
    treeView->setSortingEnabled(true);
    treeView->sortByColumn(1, Qt::DescendingOrder); // default: size desc
    treeView->setHeaderHidden(false);
    treeView->header()->setSectionResizeMode(0, QHeaderView::Fixed);
    treeView->header()->resizeSection(0, 180); // bar width similar to QDirStat
    treeView->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    treeView->header()->resizeSection(1, 60); // numeric percent
    treeView->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    treeView->setAlternatingRowColors(true);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    // Slightly taller rows to improve bar readability
    treeView->setStyleSheet("QTreeView::item{ height: 26px; }");
    
    // Create right panel with stacked widget
    rightPanel = new QStackedWidget(this);
    
    // Create visualization widgets
    sunburstWidget = new SunburstWidget(this);
    treemapWidget = new TreemapWidget(this);
    rightPanel->setStyleSheet("background:#1e1e1e;");
    
    // Add widgets to stacked widget
    rightPanel->addWidget(sunburstWidget);
    rightPanel->addWidget(treemapWidget);
    
    // Add to splitter
    mainSplitter->addWidget(treeView);
    mainSplitter->addWidget(rightPanel);
    
    // Set splitter proportions (30% left, 70% right)
    mainSplitter->setSizes({300, 900});
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);
    mainLayout->setContentsMargins(0, 0, 0, 0);
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    scanAction = fileMenu->addAction("&Scan Directory...");
    scanAction->setShortcut(QKeySequence::Open);
    scanAction->setIcon(QIcon(":/icons/scan.png"));
    
    fileMenu->addSeparator();
    
    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");
    
    sunburstAction = viewMenu->addAction("&Sunburst View");
    sunburstAction->setCheckable(true);
    sunburstAction->setChecked(true);
    sunburstAction->setShortcut(QKeySequence("Ctrl+1"));
    
    treemapAction = viewMenu->addAction("&Treemap View");
    treemapAction->setCheckable(true);
    treemapAction->setShortcut(QKeySequence("Ctrl+2"));
    
    viewMenu->addSeparator();
    
    refreshAction = viewMenu->addAction("&Refresh");
    refreshAction->setShortcut(QKeySequence::Refresh);
    refreshAction->setIcon(QIcon(":/icons/refresh.png"));
    
    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    
    QAction* clearCacheAction = toolsMenu->addAction("Clear &Cache");
    connect(clearCacheAction, &QAction::triggered, [this]() {
        ScannerWrapper::clearCache(currentPath);
        QMessageBox::information(this, "Cache Cleared", "Cache has been cleared for the current path.");
    });
    
    // Language menu
    QMenu* langMenu = menuBar()->addMenu("&Language");
    langEnglishAction = langMenu->addAction("English");
    langPortugueseAction = langMenu->addAction("Português");
    langSpanishAction = langMenu->addAction("Español");
    langEnglishAction->setCheckable(true);
    langPortugueseAction->setCheckable(true);
    langSpanishAction->setCheckable(true);
    langEnglishAction->setChecked(true);
    connect(langEnglishAction, &QAction::triggered, [this]{ appLanguage = LangEnglish; applyLanguage(); });
    connect(langPortugueseAction, &QAction::triggered, [this]{ appLanguage = LangPortuguese; applyLanguage(); });
    connect(langSpanishAction, &QAction::triggered, [this]{ appLanguage = LangSpanish; applyLanguage(); });

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    howToUseAction = helpMenu->addAction("How to &Use");
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(howToUseAction, &QAction::triggered, this, &MainWindow::onShowHelpGuide);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About DiskScout GUI", 
            "DiskScout GUI v2.0\n\n"
            "Ultra-fast disk space analyzer with modern GUI.\n"
            "Built with C+Assembly backend for maximum performance.\n\n"
            "Features:\n"
            "• Lightning-fast scanning\n"
            "• Interactive visualizations\n"
            "• File management operations\n"
            "• Cache system for instant results");
    });
}

void MainWindow::setupToolBar()
{
    topToolBar = addToolBar("Main Toolbar");
    topToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    topToolBar->setMovable(false);
    topToolBar->setStyleSheet("QToolBar{background:#2b2b2b;border:0;} QPushButton{background:#3a3a3a;color:#ddd;border:1px solid #444;padding:4px 8px;} QPushButton:checked{background:#555;} QLabel{color:#bbb;} QComboBox{background:#2b2b2b;color:#eee;border:1px solid #444;padding:2px;}");
    
    // Path selection
    pathComboBox = new QComboBox(this);
    pathComboBox->setEditable(true);
    pathComboBox->setMinimumWidth(260);
    // Populate with available root paths/drives
    QStringList roots;
    for (const QStorageInfo &si : QStorageInfo::mountedVolumes()) {
        if (si.isValid() && si.isReady()) {
            QString root = QDir(si.rootPath()).absolutePath();
            if (!roots.contains(root)) roots << root;
        }
    }
    roots.sort();
    if (roots.isEmpty()) roots << QDir::rootPath();
    pathComboBox->addItems(roots);

    // Browse button
    browseButton = new QPushButton("…", this);
    browseButton->setToolTip("Browse for folder");
    
    // Scan button
    scanButton = new QPushButton("Scan", this);
    scanButton->setIcon(QIcon(":/icons/scan.png"));
    
    // Refresh button
    refreshButton = new QPushButton("Refresh", this);
    refreshButton->setIcon(QIcon(":/icons/refresh.png"));
    
    // View mode button
    viewModeButton = new QPushButton("Sunburst", this);
    viewModeButton->setCheckable(true);
    viewModeButton->setChecked(true);
    
    // Add to toolbar
    topToolBar->addWidget(new QLabel("Path:"));
    topToolBar->addWidget(pathComboBox);
    topToolBar->addSeparator();
    topToolBar->addWidget(browseButton);
    topToolBar->addWidget(scanButton);
    topToolBar->addWidget(refreshButton);
    topToolBar->addSeparator();
    topToolBar->addWidget(viewModeButton);
}

void MainWindow::setupStatusBar()
{
    statusBar = QMainWindow::statusBar();
    
    // Progress bar
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setMaximumWidth(200);
    
    // Status labels
    statusLabel = new QLabel("Ready", this);
    sizeLabel = new QLabel("", this);
    fileCountLabel = new QLabel("", this);
    
    // Add to status bar
    statusBar->addWidget(statusLabel);
    statusBar->addPermanentWidget(progressBar);
    statusBar->addPermanentWidget(sizeLabel);
    statusBar->addPermanentWidget(fileCountLabel);
}

void MainWindow::setupConnections()
{
    // Toolbar connections
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(pathComboBox, &QComboBox::currentTextChanged, this, &MainWindow::onPathChanged);
    connect(pathComboBox, &QComboBox::editTextChanged, this, &MainWindow::onPathChanged);
    if (pathComboBox->lineEdit()) {
        connect(pathComboBox->lineEdit(), &QLineEdit::returnPressed, this, &MainWindow::onScanClicked);
    }
    connect(viewModeButton, &QPushButton::clicked, this, &MainWindow::onViewModeChanged);
    
    // Menu connections
    connect(scanAction, &QAction::triggered, this, &MainWindow::onScanClicked);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onScanClicked);
    connect(sunburstAction, &QAction::triggered, this, &MainWindow::onViewModeChanged);
    connect(treemapAction, &QAction::triggered, this, &MainWindow::onViewModeChanged);
    
    // Tree view connections
    connect(treeView, &QTreeView::customContextMenuRequested, 
            this, &MainWindow::onContextMenuRequested);
    connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current){
        if (!current.isValid()) return;
        QModelIndex sourceIdx = sortProxy ? sortProxy->mapToSource(current) : current;
        QString path = fileSystemModel->getPath(sourceIdx);
        if (!path.isEmpty()) {
            // Keep datasets as-is; request a zoom to the selected path
            sunburstWidget->zoomToPath(path);
        }
    });
    
    // Progress timer
    progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, [this]() {
        if (scanThread && scanThread->isRunning()) {
            int progress = ScannerWrapper::getScanProgress();
            if (progress <= 0) {
                // fallback to backend bytes-based heuristic percent
                progress = backend_get_progress_percent();
            }
            progressBar->setValue(qBound(0, progress, 99));
            QString p = ScannerWrapper::getProgressPath();
            if (!p.isEmpty()) {
                statusLabel->setText(QString("Scanning: %1").arg(p));
            } else {
                statusLabel->setText("Scanning...");
            }
        }
    });
}

void MainWindow::onScanClicked()
{
    // Ensure progress UI/timer from previous scan is stopped before starting another
    if (progressTimer) progressTimer->stop();
    currentPath = pathComboBox->currentText();
    
    if (currentPath.isEmpty()) {
        QMessageBox::warning(this, "Invalid Path", "Please enter a valid path to scan.");
        return;
    }
    
    // Check if path exists
    QDir dir(currentPath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "Path Not Found", "The specified path does not exist.");
        return;
    }
    
    // Start scanning
    statusLabel->setText("Scanning...");
    progressBar->setVisible(true);
    progressBar->setValue(0);
    scanButton->setEnabled(false);
    refreshButton->setEnabled(false);
    
    // Create and start scan thread
    // Stop any previous scan safely
    if (scanThread) {
        if (scanThread->isRunning()) {
            ScannerWrapper::cancelScan();
            scanThread->quit();
            scanThread->wait();
        }
        if (scanThread) scanThread->deleteLater();
        scanThread = nullptr; // QPointer resets automatically if deleted elsewhere
    }

    scanThread = new ScanThread(currentPath, this);
    connect(scanThread, &ScanThread::scanCompleted, this, &MainWindow::onScanCompleted, Qt::QueuedConnection);
    connect(scanThread, &ScanThread::scanError, this, &MainWindow::onScanError, Qt::QueuedConnection);
    connect(scanThread, &ScanThread::progressUpdated, this, &MainWindow::onScanProgress, Qt::QueuedConnection);
    // Do NOT auto-delete on finished; we delete after consuming results
    
    scanThread->start();
    if (progressTimer) progressTimer->start(100); // Update progress every 100ms
}

void MainWindow::onPathChanged()
{
    currentPath = pathComboBox->currentText();
}

void MainWindow::onBrowseClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select folder to scan", currentPath.isEmpty() ? QDir::rootPath() : currentPath,
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        pathComboBox->setCurrentText(QDir::toNativeSeparators(dir));
        // Do not auto-start a scan here; user will click Scan explicitly
    }
}

void MainWindow::onViewModeChanged()
{
    if (sunburstAction->isChecked()) {
        rightPanel->setCurrentWidget(sunburstWidget);
        viewModeButton->setText("Sunburst");
        viewModeButton->setChecked(true);
        treemapAction->setChecked(false);
    } else {
        rightPanel->setCurrentWidget(treemapWidget);
        viewModeButton->setText("Treemap");
        viewModeButton->setChecked(false);
        sunburstAction->setChecked(false);
    }
}

void MainWindow::onScanCompleted()
{
    progressTimer->stop();
    progressBar->setVisible(false);
    scanButton->setEnabled(true);
    refreshButton->setEnabled(true);
    
    // Pull results from thread
    if (scanThread) {
        directories = scanThread->getDirectories();
        totalSize = scanThread->getTotalSize();
        totalFileCount = scanThread->getTotalFileCount();
        totalDirCount = scanThread->getTotalDirCount();
        // Safe cleanup
        scanThread->deleteLater();
        scanThread = nullptr;
    }
    statusLabel->setText("Scan completed");
    sizeLabel->setText(QString("Total: %1").arg(formatSize(totalSize)));
    fileCountLabel->setText(QString("Files: %1 | Dirs: %2").arg(totalFileCount).arg(totalDirCount));
    
    updateView();
}

void MainWindow::onScanProgress(int progress)
{
    progressBar->setValue(progress);
}

void MainWindow::onScanError(const QString& error)
{
    progressTimer->stop();
    progressBar->setVisible(false);
    scanButton->setEnabled(true);
    refreshButton->setEnabled(true);
    
    statusLabel->setText("Scan failed");
    QMessageBox::critical(this, "Scan Error", error);
    if (scanThread) {
        scanThread->deleteLater();
        scanThread = nullptr;
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Ensure thread stops cleanly to avoid the app hanging on exit
    if (scanThread && scanThread->isRunning()) {
        ScannerWrapper::cancelScan();
        scanThread->quit();
        if (!scanThread->wait(3000)) {
            scanThread->terminate();
            scanThread->wait();
        }
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::onTreeSelectionChanged()
{
    // Update visualization based on tree selection
    updateView();
}

void MainWindow::onContextMenuRequested(const QPoint& pos)
{
    showContextMenu(pos);
}

void MainWindow::onOpenInExplorer()
{
    QModelIndex index = treeView->currentIndex();
    if (index.isValid()) {
        QString path = index.data(Qt::UserRole).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::onDeleteFile()
{
    QModelIndex index = treeView->currentIndex();
    if (index.isValid()) {
        QString path = index.data(Qt::UserRole).toString();
        
        int ret = QMessageBox::question(this, "Delete File", 
            QString("Are you sure you want to delete:\n%1").arg(path),
            QMessageBox::Yes | QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            QFileInfo fileInfo(path);
            if (fileInfo.isDir()) {
                QDir dir(path);
                if (dir.removeRecursively()) {
                    QMessageBox::information(this, "Success", "Directory deleted successfully.");
                    onScanClicked(); // Refresh scan
                } else {
                    QMessageBox::warning(this, "Error", "Failed to delete directory.");
                }
            } else {
                if (QFile::remove(path)) {
                    QMessageBox::information(this, "Success", "File deleted successfully.");
                    onScanClicked(); // Refresh scan
                } else {
                    QMessageBox::warning(this, "Error", "Failed to delete file.");
                }
            }
        }
    }
}

void MainWindow::onShowProperties()
{
    QModelIndex index = treeView->currentIndex();
    if (index.isValid()) {
        QString path = index.data(Qt::UserRole).toString();
        QFileInfo fileInfo(path);
        
        QString info = QString(
            "Path: %1\n"
            "Size: %2\n"
            "Type: %3\n"
            "Modified: %4\n"
            "Permissions: %5"
        ).arg(path)
         .arg(formatSize(fileInfo.size()))
         .arg(fileInfo.isDir() ? "Directory" : "File")
         .arg(fileInfo.lastModified().toString())
         .arg(fileInfo.permissions());
         
        QMessageBox::information(this, "Properties", info);
    }
}

void MainWindow::onShowHelpGuide()
{
    // Build a small help dialog with rich text
    QDialog dlg(this);
    dlg.setWindowTitle("How to Use DiskScout");
    dlg.resize(700, 520);
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QTextBrowser *doc = new QTextBrowser(&dlg);
    doc->setOpenExternalLinks(true);
    doc->setStyleSheet("QTextBrowser{background:#1e1e1e;color:#ddd;border:1px solid #444;padding:8px;}");

    QString html;
    html += "<h2>Getting Started</h2>";
    html += "<ol>"
            "<li><b>Select a path</b> from the Path box or click Browse.</li>"
            "<li>Click <b>Scan</b>. The left panel fills with folders sorted by size.</li>"
            "<li>Switch views with the toolbar (Sunburst / Treemap).</li>"
            "</ol>";
    html += "<h3>Left Panel</h3>";
    html += "<ul>"
            "<li><b>Subtree Percentage</b> bar shows share of total usage.</li>"
            "<li>Click a row to focus that folder in the visualizations.</li>"
            "<li>Right-click for actions: Open in Explorer, Delete, Properties.</li>"
            "</ul>";
    html += "<h3>Sunburst (Baobab) View</h3>";
    html += "<ul>"
            "<li>Center shows <b>Physical size</b> of the drive.</li>"
            "<li>Top bar shows <b>Logical size</b> of the current folder.</li>"
            "<li><b>Left click</b> a slice to zoom in, <b>Right click</b> to go up, use breadcrumbs to jump.</li>"
            "</ul>";
    html += "<h3>Treemap View</h3>";
    html += "<ul>"
            "<li>Toggle color mode on the legend: <b>Type colors</b> or <b>Hierarchy colors</b>.</li>"
            "<li>Hover for path/size/percent. <b>Left click</b> to drill in, <b>Right click</b> for Open/Copy/Zoom menu.</li>"
            "</ul>";
    html += "<h3>Tips</h3>";
    html += "<ul>"
            "<li>Use the Refresh button to rescan after changes.</li>"
            "<li>Cache speeds up repeat scans of the same path.</li>"
            "</ul>";

    doc->setHtml(html);
    layout->addWidget(doc);
    QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(btns);
    dlg.exec();
}

void MainWindow::updateView()
{
    // Update tree view model
    fileSystemModel->setDirectoryData(directories, totalSize);
    if (sortProxy) sortProxy->invalidate();
    // Update visualization widgets
    sunburstWidget->setRootPath(currentPath);
    sunburstWidget->updateData(directories, totalSize);
    treemapWidget->updateData(directories, totalSize);
}

void MainWindow::applyLanguage()
{
    // Very lightweight runtime text switching (no .qm yet)
    // Toolbar button titles and menu captions
    if (appLanguage == LangPortuguese) {
        viewModeButton->setText(rightPanel->currentWidget() == sunburstWidget ? "Rosácea" : "Mapa de Árvores");
        scanButton->setText("Escanear");
        refreshButton->setText("Atualizar");
        topToolBar->setWindowTitle("Barra Principal");
        langEnglishAction->setChecked(false); langPortugueseAction->setChecked(true); langSpanishAction->setChecked(false);
    } else if (appLanguage == LangSpanish) {
        viewModeButton->setText(rightPanel->currentWidget() == sunburstWidget ? "Roseta" : "Mapa de Árboles");
        scanButton->setText("Escanear");
        refreshButton->setText("Actualizar");
        topToolBar->setWindowTitle("Barra Principal");
        langEnglishAction->setChecked(false); langPortugueseAction->setChecked(false); langSpanishAction->setChecked(true);
    } else {
        viewModeButton->setText(rightPanel->currentWidget() == sunburstWidget ? "Sunburst" : "Treemap");
        scanButton->setText("Scan");
        refreshButton->setText("Refresh");
        topToolBar->setWindowTitle("Main Toolbar");
        langEnglishAction->setChecked(true); langPortugueseAction->setChecked(false); langSpanishAction->setChecked(false);
    }

    // Menu labels: we keep menu object names but could rebuild strings similarly
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    if (!index.isValid()) return;
    
    QMenu contextMenu(this);
    
    openExplorerAction = contextMenu.addAction("Open in Explorer");
    contextMenu.addSeparator();
    deleteAction = contextMenu.addAction("Delete");
    contextMenu.addSeparator();
    propertiesAction = contextMenu.addAction("Properties");
    
    connect(openExplorerAction, &QAction::triggered, this, &MainWindow::onOpenInExplorer);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteFile);
    connect(propertiesAction, &QAction::triggered, this, &MainWindow::onShowProperties);
    
    contextMenu.exec(treeView->mapToGlobal(pos));
}

// PercentageBarDelegate implementation
void PercentageBarDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    // Draw background
    painter->save();
    painter->fillRect(opt.rect, opt.state & QStyle::State_Selected ? QColor(60,90,140,80) : QColor(30,30,30));
    // Fetch percent from model (0..100)
    int pct = index.model()->data(index, FileSystemModel::BarPercentRole).toInt();
    pct = qBound(0, pct, 100);
    // Bar rect
    // Make bar taller: reduce vertical padding
    QRect r = opt.rect.adjusted(6, 4, -6, -4);
    int w = int(r.width() * (pct / 100.0));
    QRect filled(r.left(), r.top(), w, r.height());
    painter->setPen(QPen(QColor(70,70,70)));
    painter->setBrush(QColor(80,160,255,180));
    painter->drawRect(filled);
    painter->setPen(QPen(QColor(90,90,90)));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(r);
    // Percent text
    // Percent text with subtle shadow for readability
    QFont f = painter->font();
    f.setBold(true);
    painter->setFont(f);
    painter->setPen(QColor(0,0,0,180));
    painter->drawText(r.translated(1,1), Qt::AlignCenter, QString::number(pct) + "%");
    painter->setPen(Qt::white);
    painter->drawText(r, Qt::AlignCenter, QString::number(pct) + "%");
    painter->restore();
}

QString MainWindow::formatSize(uint64_t bytes)
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

// ScanThread implementation
ScanThread::ScanThread(const QString& path, QObject* parent)
    : QThread(parent), scanPath(path)
{
}

void ScanThread::run()
{
    try {
        resultDirectories.clear();
        resultTotalSize = 0;
        resultFileCount = 0;
        resultDirCount = 0;
        
        // Perform fresh scan (disable cache for now to ensure data correctness)
        if (ScannerWrapper::scanDirectory(scanPath, resultDirectories, resultTotalSize, resultFileCount, resultDirCount)) {
            emit scanCompleted();
        } else {
            emit scanError("Failed to scan directory");
        }
    } catch (const std::exception& e) {
        emit scanError(QString("Scan error: %1").arg(e.what()));
    }
}
