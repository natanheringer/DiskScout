#include "treemapwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QFileInfo>
#include <QtMath>
#include <QToolTip>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QApplication>

TreemapWidget::TreemapWidget(QWidget *parent)
    : QWidget(parent)
    , hoveredNode(nullptr)
    , selectedNode(nullptr)
    , currentRoot(nullptr)
    , isDragging(false)
    , viewOffset(0, 0)
    , scale(1.0)
    , currentDepth(0)
    , maxDepth(3)
    , animationTimer(nullptr)
    , animationProgress(0.0)
    , isAnimating(false)
{
    setMinimumSize(400, 400);
    setMouseTracking(true);
    currentRoot = &rootNode;
    currentRootPath.clear();
    legendHeight = 28;
    colorByType = false; // start with hierarchy colors for contrast
    // Ensure tooltip is white with black text (high contrast)
    setStyleSheet("QToolTip { color: #000000; background-color: #ffffff; border: 1px solid #888888; }");
    QPalette tipPal = QToolTip::palette();
    tipPal.setColor(QPalette::ToolTipBase, QColor(255,255,255));
    tipPal.setColor(QPalette::ToolTipText, QColor(0,0,0));
    QToolTip::setPalette(tipPal);
    
    // Initialize file type colors
    fileTypeColors = {
        QColor(255, 0, 0),     // Red - Executables
        QColor(255, 255, 0),   // Yellow - Images
        QColor(0, 255, 0),     // Green - Videos
        QColor(0, 0, 255),     // Blue - Audio
        QColor(255, 0, 255),   // Magenta - Documents
        QColor(255, 165, 0),   // Orange - Archives
        QColor(90, 90, 100),   // Darker neutral for Directories
        QColor(200, 200, 200)  // Light Gray - Others
    };
    
    // Setup animation timer
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &TreemapWidget::updateAnimation);

    // vivid palette for high contrast (WinDirStat-like)
    vividPalette = {
        QColor(0, 128, 255), QColor(255, 96, 0), QColor(0, 200, 120), QColor(200, 0, 200),
        QColor(255, 60, 120), QColor(120, 200, 0), QColor(255, 200, 0), QColor(80, 160, 255)
    };
    patternStyles = {
        Qt::SolidPattern, Qt::Dense3Pattern, Qt::Dense5Pattern, Qt::Dense7Pattern,
        Qt::DiagCrossPattern, Qt::Dense4Pattern, Qt::BDiagPattern, Qt::FDiagPattern
    };
}

TreemapWidget::~TreemapWidget()
{
    if (animationTimer) {
        animationTimer->stop();
    }
}

void TreemapWidget::updateData(const std::vector<ScannerWrapper::DirectoryInfo>& directories, uint64_t totalSize)
{
    Q_UNUSED(totalSize);
    buildTreemapTree(directories);
    updateLayout();
    
    // Start animation
    isAnimating = true;
    animationProgress = 0.0;
    animationTimer->start(16); // ~60 FPS
    
    update();
}

void TreemapWidget::setRootPath(const QString& path)
{
    rootPath = path;
}

void TreemapWidget::resetView()
{
    scale = 1.0;
    currentDepth = 0;
    viewOffset = QPointF(0, 0);
    hoveredNode = nullptr;
    selectedNode = nullptr;
    update();
}

void TreemapWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Clear background
    painter.fillRect(rect(), QColor(25, 25, 25));
    
    // Draw legend
    drawLegend(painter);

    // Draw breadcrumbs
    drawBreadcrumbs(painter);

    // Draw treemap
    drawTreemap(painter);
    
    // Draw labels
    drawLabels(painter);
}

void TreemapWidget::mousePressEvent(QMouseEvent* event)
{
    // Handle breadcrumb clicks first
    for (const auto &pair : breadcrumbHit) {
        if (pair.second.contains(event->pos())) {
            TreemapNode* found = findByFullPath(rootNode, pair.first);
            currentRoot = found ? found : &rootNode;
            currentRootPath = pair.first;
            updateLayout();
            update();
            return;
        }
    }
    if (event->button() == Qt::LeftButton) {
        // Toggle color mode if clicking toggle
        if (modeToggleRect.contains(event->pos())) {
            colorByType = !colorByType;
            update();
            return;
        }
        TreemapNode* node = findNodeAt(event->pos());
        if (node && node != currentRoot) {
            if (!node->children.empty()) {
                currentRoot = node;
                currentRootPath = node->fullPath;
                updateLayout();
                update();
            }
            selectedNode = node;
        }
    } else if (event->button() == Qt::RightButton) {
        TreemapNode* node = findNodeAt(event->pos());
        if (node) {
            selectedNode = node;
            QMenu menu(this);
            QString full = node->fullPath.isEmpty() ? rootPath : node->fullPath;
            QAction *openAct = menu.addAction("Open folder");
            QAction *copyAct = menu.addAction("Copy path");
            QAction *zoomInAct = nullptr;
            QAction *zoomOutAct = nullptr;
            if (!node->children.empty()) zoomInAct = menu.addAction("Zoom into");
            if (currentRoot && currentRoot->parent) zoomOutAct = menu.addAction("Zoom out");
            QAction *chosen = menu.exec(event->globalPos());
            if (chosen == openAct) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(full));
            } else if (chosen == copyAct) {
                QApplication::clipboard()->setText(full);
            } else if (chosen == zoomInAct) {
                currentRoot = node;
                currentRootPath = node->fullPath;
                updateLayout();
                update();
            } else if (chosen == zoomOutAct) {
                if (currentRoot && currentRoot->parent) {
                    currentRoot = currentRoot->parent;
                    currentRootPath = currentRoot->fullPath;
                    updateLayout();
                    update();
                }
            }
        }
    }
    else if (event->button() == Qt::RightButton) {
        if (currentRoot && currentRoot->parent) {
            currentRoot = currentRoot->parent;
            currentRootPath = currentRoot->fullPath;
            updateLayout();
            update();
        }
    }
}

void TreemapWidget::mouseMoveEvent(QMouseEvent* event)
{
    mousePos = event->pos();
    
    TreemapNode* node = findNodeAt(event->pos());
    if (node != hoveredNode) {
        hoveredNode = node;
        if (hoveredNode) {
            showTooltipForNode(hoveredNode, event->globalPos());
        } else {
            QToolTip::hideText();
        }
        update();
    }
}

void TreemapWidget::wheelEvent(QWheelEvent* event)
{
    double scaleFactor = 1.1;
    if (event->angleDelta().y() < 0) {
        scaleFactor = 1.0 / scaleFactor;
    }
    
    scale *= scaleFactor;
    scale = qBound(0.1, scale, 5.0);
    
    update();
}

void TreemapWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
    updateLayout();
}

void TreemapWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    hoveredNode = nullptr;
    update();
}

void TreemapWidget::buildTreemapTree(const std::vector<ScannerWrapper::DirectoryInfo>& directories)
{
    rootNode = TreemapNode();
    rootNode.name = "Root";
    rootNode.fullPath = "";
    rootNode.size = 0;
    rootNode.depth = 0;
    rootNode.isVisible = true;
    rootNode.parent = nullptr;
    
    QString base = normalizePath(rootPath);
    for (const auto& d : directories) {
        rootNode.size += d.size;
        QString p = normalizePath(d.path);
        if (!base.isEmpty() && p.startsWith(base)) p = p.mid(base.length());
        QStringList parts = p.split('/', Qt::SkipEmptyParts);
        addPath(rootNode, parts, 0, d.size, normalizePath(d.path));
    }
    fixParentPointers(rootNode);
    assignColors();
}

void TreemapWidget::drawTreemap(QPainter& painter)
{
    if (rootNode.children.empty()) {
        return;
    }
    
    TreemapNode* rootToDraw = currentRoot ? currentRoot : &rootNode;
    // Draw recursively with outlines for strong separation
    for (const auto& child : rootToDraw->children) {
        drawNode(painter, child, 5);
    }
}

void TreemapWidget::drawNode(QPainter& painter, const TreemapNode& node, int depthLimit)
{
    if (!node.isVisible) return;

    QRect drawRect = node.rect;
    if (isAnimating) {
        int width = static_cast<int>(node.rect.width() * animationProgress);
        int height = static_cast<int>(node.rect.height() * animationProgress);
        drawRect = QRect(node.rect.x(), node.rect.y(), width, height);
    }

    QColor color = colorByType ? getFileTypeColor(node.fullPath) : node.color;
    if (!colorByType) {
        // Ensure vivid hierarchy colors (fallbacks if assignColors not applied)
        if (!node.fullPath.isEmpty()) {
            int h = (qHash(node.fullPath) % 360);
            color = QColor::fromHsv(h, 180, 220);
        }
    }
    // Apply per-filetype brightness variants to avoid large flat blocks
    QFileInfo fi(node.fullPath);
    int variant = 0;
    if (fi.isDir()) variant = 2; // directories slightly lighter by default
    else {
        const QString ext = fi.suffix().toLower();
        if (ext == "exe" || ext == "dll") variant = 0;
        else if (ext == "jpg" || ext == "png" || ext == "gif") variant = 1;
        else if (ext == "mp4" || ext == "avi" || ext == "mkv") variant = 2;
        else if (ext == "mp3" || ext == "wav" || ext == "flac") variant = 1;
        else if (ext == "pdf" || ext == "doc" || ext == "txt") variant = 2;
        else if (ext == "zip" || ext == "rar" || ext == "7z") variant = 1;
        else variant = 0;
    }
    switch (variant) {
        case 1: color = color.lighter(110); break;
        case 2: color = color.lighter(125); break;
        default: break;
    }
    if (&node == hoveredNode) color = color.lighter(120);
    if (&node == selectedNode) color = color.lighter(150);

    QBrush brush(color);
    // Pattern follows the same file-type bucket only if colorByType mode is on
    if (colorByType) {
        int patternIndex = 0;
        QFileInfo fi(node.fullPath);
        if (fi.isDir()) patternIndex = 6;
        else {
            const QString ext = fi.suffix().toLower();
            if (ext == "exe" || ext == "dll") patternIndex = 0;
            else if (ext == "jpg" || ext == "png" || ext == "gif") patternIndex = 1;
            else if (ext == "mp4" || ext == "avi" || ext == "mkv") patternIndex = 2;
            else if (ext == "mp3" || ext == "wav" || ext == "flac") patternIndex = 3;
            else if (ext == "pdf" || ext == "doc" || ext == "txt") patternIndex = 4;
            else if (ext == "zip" || ext == "rar" || ext == "7z") patternIndex = 5;
            else patternIndex = 7;
        }
        brush.setStyle(patternStyles[patternIndex % patternStyles.size()]);
    } else {
        brush.setStyle(Qt::SolidPattern);
    }
    painter.setBrush(brush);
    // White inner stroke for crisp separation + dark outer grid
    painter.setPen(QPen(QColor(255,255,255,35), 2));
    painter.drawRect(drawRect);
    painter.setPen(QPen(QColor(10,10,10), 1));
    painter.drawRect(drawRect.adjusted(1,1,-1,-1));

    if (depthLimit <= 0) return;
    for (const auto& ch : node.children) {
        drawNode(painter, ch, depthLimit - 1);
    }
}

void TreemapWidget::drawLabels(QPainter& painter)
{
    if (rootNode.children.empty()) {
        return;
    }
    
    painter.setFont(QFont("Arial", 8, QFont::Bold));
    
    TreemapNode* rootToDraw = currentRoot ? currentRoot : &rootNode;
    for (const auto& child : rootToDraw->children) {
        if (!child.isVisible) continue;
        
        QRect drawRect = child.rect;
        
        // Apply animation
        if (isAnimating) {
            int width = static_cast<int>(child.rect.width() * animationProgress);
            int height = static_cast<int>(child.rect.height() * animationProgress);
            drawRect = QRect(child.rect.x(), child.rect.y(), width, height);
        }
        
        // Only draw labels if rectangle is large enough
        if (drawRect.width() > 50 && drawRect.height() > 20) {
            QString label = child.name + "  " + formatSize(child.size);
            QColor bg = getNodeColor(child);
            QColor textColor = getContrastingTextColor(bg);
            QColor shadow = (textColor.lightness() < 128) ? QColor(255,255,255,190) : QColor(0,0,0,190);
            painter.setBrush(Qt::NoBrush);
            // Shadow for readability
            painter.setPen(shadow);
            painter.drawText(drawRect.adjusted(5, 3, -3, -1), Qt::AlignLeft | Qt::AlignTop, label);
            // Main text
            painter.setPen(textColor);
            painter.drawText(drawRect.adjusted(4, 2, -4, -2), Qt::AlignLeft | Qt::AlignTop, label);
        }
    }
}

QColor TreemapWidget::getFileTypeColor(const QString& path) const
{
    QFileInfo fileInfo(path);
    QString extension = fileInfo.suffix().toLower();
    
    if (fileInfo.isDir()) {
        return fileTypeColors[6]; // Gray for directories
    } else if (extension == "exe" || extension == "dll") {
        return fileTypeColors[0]; // Red for executables
    } else if (extension == "jpg" || extension == "png" || extension == "gif") {
        return fileTypeColors[1]; // Yellow for images
    } else if (extension == "mp4" || extension == "avi" || extension == "mkv") {
        return fileTypeColors[2]; // Green for videos
    } else if (extension == "mp3" || extension == "wav" || extension == "flac") {
        return fileTypeColors[3]; // Blue for audio
    } else if (extension == "pdf" || extension == "doc" || extension == "txt") {
        return fileTypeColors[4]; // Magenta for documents
    } else if (extension == "zip" || extension == "rar" || extension == "7z") {
        return fileTypeColors[5]; // Orange for archives
    } else {
        return fileTypeColors[7]; // Light gray for others
    }
}

QString TreemapWidget::formatSize(uint64_t bytes) const
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

QColor TreemapWidget::getNodeColor(const TreemapNode& node) const
{
    QColor color = colorByType ? getFileTypeColor(node.fullPath) : node.color;
    if (!colorByType) {
        if (!node.fullPath.isEmpty()) {
            int h = (qHash(node.fullPath) % 360);
            color = QColor::fromHsv(h, 180, 220);
        }
    }
    // Apply same brightness variants as paint path for consistency
    QFileInfo fi(node.fullPath);
    int variant = 0;
    if (fi.isDir()) variant = 2; else {
        const QString ext = fi.suffix().toLower();
        if (ext == "jpg" || ext == "png" || ext == "gif" || ext == "zip" || ext == "rar" || ext == "7z") variant = 1;
        else if (ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "pdf" || ext == "doc" || ext == "txt") variant = 2;
    }
    switch (variant) { case 1: color = color.lighter(110); break; case 2: color = color.lighter(125); break; default: break; }
    return color;
}

QColor TreemapWidget::getContrastingTextColor(const QColor& bg) const
{
    // Compute luminance; threshold ~150 for bold backgrounds
    double L = 0.2126*bg.red() + 0.7152*bg.green() + 0.0722*bg.blue();
    return (L > 140.0) ? QColor(20,20,20) : QColor(245,245,245);
}

TreemapWidget::TreemapNode* TreemapWidget::findNodeAt(const QPoint& point)
{
    TreemapNode* rootToSearch = currentRoot ? currentRoot : &rootNode;
    return findNodeAtRecursive(*rootToSearch, point);
}

TreemapWidget::TreemapNode* TreemapWidget::findNodeAtRecursive(TreemapNode& node, const QPoint& point)
{
    for (auto& ch : node.children) {
        if (ch.rect.contains(point)) {
            // Prefer deepest match
            TreemapNode* deeper = findNodeAtRecursive(ch, point);
            return deeper ? deeper : &ch;
        }
    }
    return nullptr;
}

void TreemapWidget::updateLayout()
{
    if (rootNode.children.empty()) {
        return;
    }
    
    // Calculate available space (reserve legend at top)
    QRect availableRect = rect().adjusted(10, 10 + legendHeight + 24, -10, -10);
    
    // Apply squarified treemap algorithm starting at currentRoot
    ensureCurrentRootValid();
    TreemapNode* rootToLayout = currentRoot ? currentRoot : &rootNode;
    squarifyTreemap(*rootToLayout, availableRect);
    
    update();
}

void TreemapWidget::squarifyTreemap(TreemapNode& node, const QRect& rect)
{
    if (node.children.empty()) {
        return;
    }
    
    // Sort children by size (largest first)
    std::sort(node.children.begin(), node.children.end(),
              [](const TreemapNode& a, const TreemapNode& b) {
                  return a.size > b.size;
              });

    // Compute total
    double total = 0.0;
    for (const auto& c : node.children) total += double(c.size);
    if (total <= 0.0) return;

    // Squarify using alternating row/column packing (simple variant)
    QRectF free = rect;
    bool horizontal = free.width() >= free.height();
    int i = 0;
    while (i < int(node.children.size())) {
        // Build a row with items until aspect ratio would worsen
        std::vector<int> rowIdx; rowIdx.reserve(8);
        double rowSum = 0.0;

        // Simple batching without full worst() math for now (good enough)
        while (i < int(node.children.size())) {
            rowIdx.push_back(i);
            rowSum += double(node.children[i].size);
            if (rowIdx.size() >= 6) { ++i; break; }
            ++i;
            if (rowSum / total >= 0.25) break; // keep rows balanced
        }

        // Lay out the row
        double rowArea = (rowSum / total) * (rect.width() * rect.height());
        if (horizontal) {
            double rowHeight = qMax(1.0, rowArea / free.width());
            double x = free.left();
            for (int idx : rowIdx) {
                double frac = double(node.children[idx].size) / rowSum;
                double w = frac * free.width();
                node.children[idx].rect = QRect(int(x), int(free.top()), int(w), int(rowHeight));
                x += w;
                // Recurse
                if (!node.children[idx].children.empty()) {
                    squarifyTreemap(node.children[idx], node.children[idx].rect.adjusted(1,1,-1,-1));
                }
            }
            free.setTop(free.top() + rowHeight);
        } else {
            double rowWidth = qMax(1.0, rowArea / free.height());
            double y = free.top();
            for (int idx : rowIdx) {
                double frac = double(node.children[idx].size) / rowSum;
                double h = frac * free.height();
                node.children[idx].rect = QRect(int(free.left()), int(y), int(rowWidth), int(h));
                y += h;
                if (!node.children[idx].children.empty()) {
                    squarifyTreemap(node.children[idx], node.children[idx].rect.adjusted(1,1,-1,-1));
                }
            }
            free.setLeft(free.left() + rowWidth);
        }
        horizontal = !horizontal;
    }
}

void TreemapWidget::setNodeColor(TreemapNode& node)
{
    node.color = getFileTypeColor(node.fullPath);
}

void TreemapWidget::calculateNodeRects(TreemapNode& node, const QRect& rect)
{
    // Implementation for calculating node rectangles
    Q_UNUSED(node)
    Q_UNUSED(rect)
}

void TreemapWidget::updateAnimation()
{
    animationProgress += 0.05;
    if (animationProgress >= 1.0) {
        animationProgress = 1.0;
        isAnimating = false;
        animationTimer->stop();
    }
    
    update();
}

QString TreemapWidget::normalizePath(const QString& p) const
{
    QString s = p;
    s.replace('\\','/');
    if (s.endsWith('/')) s.chop(1);
    return s;
}

void TreemapWidget::addPath(TreemapNode& parent, const QStringList& parts, int index, uint64_t size, const QString& fullPath)
{
    if (index >= parts.size()) return;
    const QString &part = parts[index];
    // Find or create child
    for (auto &ch : parent.children) {
        if (ch.name == part) {
            ch.size += size;
            addPath(ch, parts, index+1, size, fullPath);
            return;
        }
    }
    TreemapNode child;
    child.name = part;
    child.fullPath = parent.fullPath.isEmpty() ? part : (parent.fullPath + "/" + part);
    child.size = size;
    child.depth = parent.depth + 1;
    child.isVisible = true;
    child.parent = &parent;
    setNodeColor(child);
    parent.children.push_back(child);
    addPath(parent.children.back(), parts, index+1, size, fullPath);
}

void TreemapWidget::fixParentPointers(TreemapNode& node)
{
    for (auto &ch : node.children) {
        ch.parent = &node;
        fixParentPointers(ch);
    }
}

void TreemapWidget::ensureCurrentRootValid()
{
    if (!currentRoot) { currentRoot = &rootNode; currentRootPath.clear(); return; }
    if (!currentRootPath.isEmpty()) {
        // Shallow re-find by path
        std::function<TreemapNode*(TreemapNode&, const QString&)> findBy = [&](TreemapNode& n, const QString& path)->TreemapNode*{
            if (n.fullPath == path) return &n;
            for (auto &ch : n.children) { if (ch.fullPath == path) return &ch; }
            for (auto &ch : n.children) { if (auto* r = findBy(ch, path)) return r; }
            return nullptr;
        };
        TreemapNode* found = findBy(rootNode, currentRootPath);
        if (found) currentRoot = found; else currentRoot = &rootNode;
    }
}

void TreemapWidget::showTooltipForNode(TreemapNode* node, const QPoint& globalPos)
{
    if (!node) return;
    // Calculate percent relative to current root
    uint64_t parentSize = currentRoot ? currentRoot->size : rootNode.size;
    double pct = parentSize ? (double(node->size) * 100.0 / double(parentSize)) : 0.0;
    QString text = QString("%1\n%2  (%3%1)")
        .arg("") // placeholder to keep translator-friendly structure
        .arg(formatSize(node->size))
        .arg(QString::number(pct, 'f', 1));
    text = (node->fullPath.isEmpty() ? rootPath : node->fullPath) + "\n" + formatSize(node->size) + QString("  (%1%)").arg(QString::number(pct, 'f', 1));
    QToolTip::showText(globalPos, text, this);
}

void TreemapWidget::assignColors()
{
    // Assign vivid base colors for first level; lighten for deeper levels
    TreemapNode* base = &rootNode;
    for (size_t i = 0; i < base->children.size(); ++i) {
        QColor baseColor = vividPalette[i % vividPalette.size()];
        base->children[i].color = baseColor;
        std::function<void(TreemapNode&, int)> tint = [&](TreemapNode& n, int depth){
            for (auto &ch : n.children) {
                int factor = 100 + depth * 12;
                ch.color = baseColor.lighter(factor);
                tint(ch, depth + 1);
            }
        };
        tint(base->children[i], 1);
    }
}

Qt::BrushStyle TreemapWidget::getPatternForNode(const TreemapNode& node) const
{
    uint32_t h = qHash(node.fullPath);
    if (patternStyles.empty()) return Qt::SolidPattern;
    return patternStyles[h % patternStyles.size()];
}

void TreemapWidget::drawLegend(QPainter& painter)
{
    QRect bg = QRect(10, 10, width() - 20, legendHeight - 10);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30,30,30));
    painter.drawRoundedRect(bg, 6, 6);

    QStringList labels = {"Executables","Images","Videos","Audio","Docs","Archives","Dirs","Other"};
    int x = 18;
    for (int i = 0; i < (int)fileTypeColors.size() && i < labels.size(); ++i) {
        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(fileTypeColors[i]);
        QRect swatch(x, bg.top()+4, 12, 12);
        painter.drawRect(swatch);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 8));
        painter.drawText(x + 16, bg.top()+14, labels[i]);
        x += 90;
        if (x > width() - 120) break;
    }

    // Mode toggle button
    QString modeText = colorByType ? "Type colors" : "Hierarchy colors";
    int w = 120; int h = 18;
    modeToggleRect = QRect(width() - w - 20, bg.top()+3, w, h);
    painter.setPen(QPen(QColor(120,120,120)));
    painter.setBrush(QColor(50,50,50));
    painter.drawRoundedRect(modeToggleRect, 6, 6);
    painter.setPen(Qt::white);
    painter.drawText(modeToggleRect, Qt::AlignCenter, modeText);
}

std::vector<QString> TreemapWidget::buildBreadcrumbPaths() const
{
    std::vector<QString> crumbs;
    const TreemapNode* n = currentRoot ? currentRoot : &rootNode;
    while (n) {
        crumbs.push_back(n->fullPath.isEmpty() ? rootPath : n->fullPath);
        n = n->parent;
    }
    std::reverse(crumbs.begin(), crumbs.end());
    return crumbs;
}

TreemapWidget::TreemapNode* TreemapWidget::findByFullPath(TreemapNode& node, const QString& path)
{
    if ((node.fullPath.isEmpty() ? rootPath : node.fullPath) == path) return &node;
    for (auto &ch : node.children) {
        if ((ch.fullPath.isEmpty() ? rootPath : ch.fullPath) == path) return &ch;
        if (auto* r = findByFullPath(ch, path)) return r;
    }
    return nullptr;
}

void TreemapWidget::drawBreadcrumbs(QPainter& painter)
{
    breadcrumbHit.clear();
    auto crumbs = buildBreadcrumbPaths();
    int y = 10 + legendHeight; int x = 14;
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.setPen(QPen(Qt::white));
    for (size_t i = 0; i < crumbs.size(); ++i) {
        QString label = (i == 0) ? QString("Root") : QFileInfo(crumbs[i]).fileName();
        QRectF r(x, y, painter.fontMetrics().horizontalAdvance(label) + 16, 18);
        painter.setBrush(QColor(55,55,55)); painter.setPen(QPen(QColor(120,120,120)));
        painter.drawRoundedRect(r, 6, 6);
        painter.setPen(Qt::white);
        painter.drawText(r.adjusted(8,0,-6,0), Qt::AlignVCenter|Qt::AlignLeft, label);
        breadcrumbHit.push_back({crumbs[i], r});
        x += int(r.width()) + 8;
        if (x > width() - 120) break;
    }
}
