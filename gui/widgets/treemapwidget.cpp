#include "treemapwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QFileInfo>
#include <QtMath>

TreemapWidget::TreemapWidget(QWidget *parent)
    : QWidget(parent)
    , hoveredNode(nullptr)
    , selectedNode(nullptr)
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
    
    // Initialize file type colors
    fileTypeColors = {
        QColor(255, 0, 0),     // Red - Executables
        QColor(255, 255, 0),   // Yellow - Images
        QColor(0, 255, 0),     // Green - Videos
        QColor(0, 0, 255),     // Blue - Audio
        QColor(255, 0, 255),   // Magenta - Documents
        QColor(255, 165, 0),   // Orange - Archives
        QColor(100, 100, 100), // Gray - Directories
        QColor(200, 200, 200)  // Light Gray - Others
    };
    
    // Setup animation timer
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &TreemapWidget::updateAnimation);
}

TreemapWidget::~TreemapWidget()
{
    if (animationTimer) {
        animationTimer->stop();
    }
}

void TreemapWidget::updateData(const std::vector<ScannerWrapper::DirectoryInfo>& directories, uint64_t totalSize)
{
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
    // Implementation for setting root path
    Q_UNUSED(path)
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
    
    // Draw treemap
    drawTreemap(painter);
    
    // Draw labels
    drawLabels(painter);
}

void TreemapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        TreemapNode* node = findNodeAt(event->pos());
        if (node) {
            selectedNode = node;
            // TODO: Implement drill-down functionality
        }
    }
}

void TreemapWidget::mouseMoveEvent(QMouseEvent* event)
{
    mousePos = event->pos();
    
    TreemapNode* node = findNodeAt(event->pos());
    if (node != hoveredNode) {
        hoveredNode = node;
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
    
    // Calculate total size
    for (const auto& dir : directories) {
        rootNode.size += dir.size;
    }
    
    // Build tree structure (simplified - just add all as children)
    for (const auto& dir : directories) {
        TreemapNode node;
        node.name = QFileInfo(dir.path).fileName();
        node.fullPath = dir.path;
        node.size = dir.size;
        node.depth = 1;
        node.isVisible = true;
        setNodeColor(node);
        rootNode.children.push_back(node);
    }
}

void TreemapWidget::drawTreemap(QPainter& painter)
{
    if (rootNode.children.empty()) {
        return;
    }
    
    // Draw root node children
    for (const auto& child : rootNode.children) {
        if (!child.isVisible) continue;
        
        QRect drawRect = child.rect;
        
        // Apply animation
        if (isAnimating) {
            int width = static_cast<int>(child.rect.width() * animationProgress);
            int height = static_cast<int>(child.rect.height() * animationProgress);
            drawRect = QRect(child.rect.x(), child.rect.y(), width, height);
        }
        
        // Draw rectangle
        QColor color = child.color;
        if (hoveredNode == &child) {
            color = color.lighter(120); // Highlight on hover
        }
        if (selectedNode == &child) {
            color = color.lighter(150); // Highlight selection
        }
        
        painter.setBrush(color);
        painter.setPen(QPen(Qt::white, 1));
        painter.drawRect(drawRect);
    }
}

void TreemapWidget::drawLabels(QPainter& painter)
{
    if (rootNode.children.empty()) {
        return;
    }
    
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 8));
    
    for (const auto& child : rootNode.children) {
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
            QString label = child.name;
            if (label.length() > 15) {
                label = label.left(12) + "...";
            }
            
            painter.drawText(drawRect, Qt::AlignCenter, label);
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

TreemapWidget::TreemapNode* TreemapWidget::findNodeAt(const QPoint& point)
{
    for (auto& child : rootNode.children) {
        if (child.rect.contains(point)) {
            return &child;
        }
    }
    return nullptr;
}

void TreemapWidget::updateLayout()
{
    if (rootNode.children.empty()) {
        return;
    }
    
    // Calculate available space
    QRect availableRect = rect().adjusted(10, 10, -10, -10);
    
    // Apply squarified treemap algorithm
    squarifyTreemap(rootNode, availableRect);
    
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
    
    // Simple treemap layout (not fully squarified, but functional)
    int x = rect.x();
    int y = rect.y();
    int width = rect.width();
    int height = rect.height();
    
    uint64_t totalSize = 0;
    for (const auto& child : node.children) {
        totalSize += child.size;
    }
    
    int currentY = y;
    int remainingHeight = height;
    
    for (size_t i = 0; i < node.children.size(); i++) {
        auto& child = node.children[i];
        
        // Calculate height based on size proportion
        int childHeight = static_cast<int>((child.size * height) / totalSize);
        childHeight = qMax(childHeight, 20); // Minimum height
        
        if (i == node.children.size() - 1) {
            // Last child takes remaining space
            childHeight = remainingHeight;
        }
        
        child.rect = QRect(x, currentY, width, childHeight);
        currentY += childHeight;
        remainingHeight -= childHeight;
        
        if (remainingHeight <= 0) break;
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
