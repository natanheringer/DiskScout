#include "sunburstwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QFileInfo>
#include <QtMath>
#include <functional>

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
    currentRoot = &rootNode;
    
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
    // Vivid palette similar to Baobab (cycling hues)
    vividPalette = {
        QColor::fromHsv( 10, 200, 220), QColor::fromHsv( 35, 200, 220),
        QColor::fromHsv( 60, 200, 220), QColor::fromHsv(120, 200, 220),
        QColor::fromHsv(160, 200, 220), QColor::fromHsv(200, 200, 220),
        QColor::fromHsv(260, 200, 220), QColor::fromHsv(300, 200, 220)
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
    Q_UNUSED(totalSize);
    buildSunburstTree(directories);
    // Precompute angles for entire tree and reset current root
    calculateNodeAngles(rootNode, 0.0, 360.0);
    currentRoot = &rootNode;
    updateLayout();
    
    // Start animation
    isAnimating = true;
    animationProgress = 0.0;
    animationTimer->start(16); // ~60 FPS
    
    update();
}

void SunburstWidget::setRootPath(const QString& path)
{
    rootPath = path;
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
        SunburstNode* node = findNodeAt(event->pos());
        if (node && node != currentRoot && !node->children.empty()) {
            currentRoot = node;
            calculateNodeAngles(*currentRoot, 0.0, 360.0);
            update();
        }
    } else if (event->button() == Qt::RightButton) {
        if (currentRoot && currentRoot->parent) {
            currentRoot = currentRoot->parent;
            calculateNodeAngles(*currentRoot, 0.0, 360.0);
            update();
        }
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
    // Build a multi-level hierarchy relative to rootPath
    rootNode = SunburstNode();
    rootNode.name = "Root";
    rootNode.fullPath = "";
    rootNode.size = 0;
    rootNode.depth = 0;

    QString base = rootPath; base.replace('\\','/');
    for (const auto& d : directories) {
        rootNode.size += d.size;
        QString p = d.path; p.replace('\\','/');
        if (!base.isEmpty() && p.startsWith(base)) p = p.mid(base.length());
        QStringList parts = p.split('/', Qt::SkipEmptyParts);
        addPath(rootNode, parts, 0, d.size, d.path);
    }

    // Color assignment: vivid for first level, tints deeper
    for (size_t i = 0; i < rootNode.children.size(); ++i) {
        QColor baseColor = vividPalette.empty() ? QColor::fromHsv(int(i*35)%360, 200, 220) : vividPalette[i % vividPalette.size()];
        rootNode.children[i].color = baseColor;
        std::function<void(SunburstNode&, int)> tint = [&](SunburstNode& n, int depth){
            for (auto &ch : n.children) {
                ch.color = baseColor.lighter(100 + depth*10);
                tint(ch, depth+1);
            }
        };
        tint(rootNode.children[i], 1);
    }
}

int SunburstWidget::getMaxDepth(const SunburstNode& node) const
{
    int md = node.depth;
    for (const auto& ch : node.children) {
        md = std::max(md, getMaxDepth(ch));
    }
    return md;
}

void SunburstWidget::drawSunburst(QPainter& painter)
{
    if (currentRoot->children.empty()) {
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
                     Qt::AlignCenter, formatSize(currentRoot->size));
    
    // Precompute angles for current root
    calculateNodeAngles(*currentRoot, 0.0, 360.0);

    // Determine ring count from depth
    int maxD = getMaxDepth(*currentRoot) - currentRoot->depth;
    maxD = qBound(1, maxD, 6);
    double ringWidth = (maxRadius - innerRadius) / maxD;

    // Draw rings depth by depth
    std::function<void(const SunburstNode&)> drawDepth = [&](const SunburstNode& node) {
        for (const auto& ch : node.children) {
            int depthIndex = ch.depth - currentRoot->depth; // 1..maxD
            if (depthIndex <= 0 || depthIndex > maxD) {
                drawDepth(ch);
                continue;
            }
            double rInner = innerRadius + ringWidth * (depthIndex - 1);
            double rOuter = rInner + ringWidth;
            QRectF rect(center.x() - rOuter, center.y() - rOuter, rOuter*2, rOuter*2);
            painter.setBrush(ch.color);
            painter.setPen(QPen(Qt::black, 1));
            painter.drawPie(rect, int(ch.startAngle * 16), int(ch.spanAngle * 16));
            drawDepth(ch);
        }
    };
    drawDepth(*currentRoot);
}

void SunburstWidget::drawLabels(QPainter& painter)
{
    if (currentRoot->children.empty()) {
        return;
    }
    
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 8));
    
    // Recompute geometry similar to drawSunburst so labels sit at a good radius
    QRectF widgetRect = rect();
    center = widgetRect.center() + viewOffset;
    double padding = 20.0;
    double maxRadius = qMin(widgetRect.width(), widgetRect.height()) / 2.0 - padding;
    double innerRadius = qMax(24.0, maxRadius * 0.18);
    int maxD = qBound(1, getMaxDepth(*currentRoot) - currentRoot->depth, 6);
    double ringWidth = (maxRadius - innerRadius) / maxD;

    std::function<void(const SunburstNode&)> drawText = [&](const SunburstNode& node) {
        for (const auto& ch : node.children) {
            int depthIndex = ch.depth - currentRoot->depth; // 1..maxD
            if (depthIndex > 0 && depthIndex <= maxD) {
                if (ch.spanAngle > 8.0) {
                    double mid = ch.startAngle + ch.spanAngle/2.0;
                    double radians = qDegreesToRadians(mid);
                    double radius = innerRadius + ringWidth * (depthIndex-0.5);
                    QPointF labelPos(center.x() + radius * qCos(radians),
                                     center.y() + radius * qSin(radians));
                    QString label = ch.name;
                    if (label.length() > 16) label = label.left(13) + "...";
                    painter.drawText(QRectF(labelPos.x() - 40, labelPos.y() - 10, 80, 20),
                                     Qt::AlignCenter, label);
                }
            }
            drawText(ch);
        }
    };
    drawText(*currentRoot);
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
    // Convert point to angle and radius relative to center
    QPointF v = point - center;
    double r = std::hypot(v.x(), v.y());
    double ang = std::atan2(v.y(), v.x());
    double deg = qRadiansToDegrees(ang);
    if (deg < 0) deg += 360.0;

    double padding = 20.0;
    double maxRadius = qMin(width(), height()) / 2.0 - padding;
    double innerRadius = qMax(24.0, maxRadius * 0.18);
    int maxD = qBound(1, getMaxDepth(*currentRoot) - currentRoot->depth, 6);
    double ringWidth = (maxRadius - innerRadius) / maxD;
    int ringIdx = int((r - innerRadius) / ringWidth) + 1; // 1..maxD

    SunburstNode* result = nullptr;
    std::function<void(SunburstNode&)> find = [&](SunburstNode& node){
        for (auto &ch : node.children) {
            int dIdx = ch.depth - currentRoot->depth;
            if (dIdx == ringIdx) {
                if (deg >= ch.startAngle && deg <= ch.startAngle + ch.spanAngle) {
                    result = &ch;
                    return;
                }
            }
            find(ch);
            if (result) return;
        }
    };
    find(*currentRoot);
    return result ? result : currentRoot;
}

void SunburstWidget::updateLayout()
{
    // Update layout when widget is resized
    update();
}

void SunburstWidget::calculateNodeAngles(SunburstNode& node, double startAngle, double spanAngle)
{
    node.startAngle = startAngle;
    node.spanAngle = spanAngle;
    double s = startAngle;
    if (node.size == 0) return;
    for (auto &ch : node.children) {
        double childSpan = (double(ch.size) / double(node.size)) * spanAngle;
        calculateNodeAngles(ch, s, childSpan);
        s += childSpan;
    }
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

void SunburstWidget::addPath(SunburstNode& root, const QStringList& parts, int idx, uint64_t size, const QString& fullPath)
{
    if (idx >= parts.size()) return;
    const QString &part = parts[idx];

    // Find or create child
    for (auto &ch : root.children) {
        if (ch.name == part) {
            ch.size += size;
            addPath(ch, parts, idx+1, size, fullPath);
            return;
        }
    }
    SunburstNode child;
    child.name = part;
    child.fullPath = fullPath;
    child.size = size;
    child.depth = root.depth + 1;
    child.parent = &root;
    setNodeColor(child);
    root.children.push_back(child);
    // Recurse deeper
    addPath(root.children.back(), parts, idx+1, size, fullPath);
}
