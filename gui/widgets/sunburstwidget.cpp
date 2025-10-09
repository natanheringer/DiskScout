#include "sunburstwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QFileInfo>
#include <QtMath>

SunburstWidget::SunburstWidget(QWidget *parent)
    : QWidget(parent)
    , scale(1.0)
    , currentDepth(0)
    , maxDepth(5)
    , isDragging(false)
    , viewOffset(0, 0)
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
    connect(animationTimer, &QTimer::timeout, this, &SunburstWidget::updateAnimation);
}

SunburstWidget::~SunburstWidget()
{
    if (animationTimer) {
        animationTimer->stop();
    }
}

void SunburstWidget::updateData(const std::vector<ScannerWrapper::DirectoryInfo>& directories, uint64_t totalSize)
{
    buildSunburstTree(directories);
    updateLayout();
    
    // Start animation
    isAnimating = true;
    animationProgress = 0.0;
    animationTimer->start(16); // ~60 FPS
    
    update();
}

void SunburstWidget::setRootPath(const QString& path)
{
    // Implementation for setting root path
    Q_UNUSED(path)
}

void SunburstWidget::resetView()
{
    scale = 1.0;
    currentDepth = 0;
    viewOffset = QPointF(0, 0);
    update();
}

void SunburstWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Clear background
    painter.fillRect(rect(), QColor(25, 25, 25));
    
    // Draw sunburst
    drawSunburst(painter);
    
    // Draw labels
    drawLabels(painter);
}

void SunburstWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        dragStart = event->pos();
    }
}

void SunburstWidget::mouseMoveEvent(QMouseEvent* event)
{
    mousePos = event->pos();
    
    if (isDragging) {
        QPointF delta = event->pos() - dragStart;
        viewOffset += delta;
        dragStart = event->pos();
        update();
    }
}

void SunburstWidget::wheelEvent(QWheelEvent* event)
{
    double scaleFactor = 1.1;
    if (event->angleDelta().y() < 0) {
        scaleFactor = 1.0 / scaleFactor;
    }
    
    scale *= scaleFactor;
    scale = qBound(0.1, scale, 5.0);
    
    update();
}

void SunburstWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
    updateLayout();
}

void SunburstWidget::buildSunburstTree(const std::vector<ScannerWrapper::DirectoryInfo>& directories)
{
    rootNode = SunburstNode();
    rootNode.name = "Root";
    rootNode.fullPath = "";
    rootNode.size = 0;
    rootNode.depth = 0;
    
    // Calculate total size
    for (const auto& dir : directories) {
        rootNode.size += dir.size;
    }
    
    // Build tree structure (simplified - just add all as children)
    for (const auto& dir : directories) {
        SunburstNode node;
        node.name = QFileInfo(dir.path).fileName();
        node.fullPath = dir.path;
        node.size = dir.size;
        node.depth = 1;
        setNodeColor(node);
        rootNode.children.push_back(node);
    }
}

void SunburstWidget::drawSunburst(QPainter& painter)
{
    if (rootNode.children.empty()) {
        return;
    }
    
    // Calculate center and radius; fill most of the right panel
    QRectF widgetRect = rect();
    center = widgetRect.center() + viewOffset;
    double padding = 20.0;
    double maxRadius = qMin(widgetRect.width(), widgetRect.height()) / 2.0 - padding;
    
    // Draw center circle
    painter.setBrush(QColor(53, 53, 53));
    painter.setPen(QPen(Qt::white, 1));
    double innerRadius = qMax(24.0, maxRadius * 0.18); // keep inner hub proportional
    painter.drawEllipse(center, innerRadius, innerRadius);
    
    // Draw center text
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(QRectF(center.x() - innerRadius, center.y() - 10, innerRadius * 2, 20), 
                     Qt::AlignCenter, "Root");
    
    // Draw rings
    double ringWidth = (maxRadius - innerRadius) / qMax(1, maxDepth);
    double currentRadius = innerRadius;
    
    for (int depth = 0; depth < maxDepth && depth < currentDepth + 1; depth++) {
        double startAngle = 0;
        double totalAngle = 360.0;
        
        if (depth == 0) {
            // Draw root children
            for (size_t i = 0; i < rootNode.children.size(); i++) {
                const auto& child = rootNode.children[i];
                double spanAngle = (child.size * totalAngle) / rootNode.size;
                
                // Apply animation
                if (isAnimating) {
                    spanAngle *= animationProgress;
                }
                
                painter.setBrush(child.color);
                painter.setPen(QPen(Qt::white, 1));
                
                QRectF ringRect(center.x() - currentRadius, center.y() - currentRadius,
                               currentRadius * 2, currentRadius * 2);
                
                painter.drawPie(ringRect, static_cast<int>(startAngle * 16), 
                               static_cast<int>(spanAngle * 16));
                
                startAngle += spanAngle;
            }
        }
        
        currentRadius += ringWidth;
    }
}

void SunburstWidget::drawLabels(QPainter& painter)
{
    if (rootNode.children.empty()) {
        return;
    }
    
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 9));
    
    // Recompute geometry similar to drawSunburst so labels sit at a good radius
    QRectF widgetRect = rect();
    center = widgetRect.center() + viewOffset;
    double padding = 20.0;
    double maxRadius = qMin(widgetRect.width(), widgetRect.height()) / 2.0 - padding;
    double innerRadius = qMax(24.0, maxRadius * 0.18);
    double labelRadius = innerRadius + (maxRadius - innerRadius) * 0.45; // mid ring

    double startAngle = 0;
    double totalAngle = 360.0;
    
    for (const auto& child : rootNode.children) {
        double spanAngle = (child.size * totalAngle) / rootNode.size;
        double midAngle = startAngle + spanAngle / 2.0;
        
        // Convert angle to radians
        double radians = qDegreesToRadians(midAngle);
        
        // Calculate label position
        double radius = labelRadius;
        QPointF labelPos(center.x() + radius * qCos(radians),
                        center.y() + radius * qSin(radians));
        
        // Draw label if there's enough space
        if (spanAngle > 8.0) { // Only draw labels for larger segments to avoid overlap
            QString label = child.name;
            if (label.length() > 15) {
                label = label.left(12) + "...";
            }
            
            painter.drawText(QRectF(labelPos.x() - 30, labelPos.y() - 10, 60, 20),
                           Qt::AlignCenter, label);
        }
        
        startAngle += spanAngle;
    }
}

QColor SunburstWidget::getFileTypeColor(const QString& path) const
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

QString SunburstWidget::formatSize(uint64_t bytes) const
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

SunburstWidget::SunburstNode* SunburstWidget::findNodeAt(const QPointF& point)
{
    // Implementation for finding node at mouse position
    Q_UNUSED(point)
    return nullptr;
}

void SunburstWidget::updateLayout()
{
    // Update layout when widget is resized
    update();
}

void SunburstWidget::calculateNodeAngles(SunburstNode& node, double startAngle, double spanAngle)
{
    // Implementation for calculating node angles
    Q_UNUSED(node)
    Q_UNUSED(startAngle)
    Q_UNUSED(spanAngle)
}

void SunburstWidget::setNodeColor(SunburstNode& node)
{
    node.color = getFileTypeColor(node.fullPath);
}

void SunburstWidget::updateAnimation()
{
    animationProgress += 0.05;
    if (animationProgress >= 1.0) {
        animationProgress = 1.0;
        isAnimating = false;
        animationTimer->stop();
    }
    
    update();
}
