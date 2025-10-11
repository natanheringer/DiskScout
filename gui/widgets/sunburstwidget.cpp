#include "sunburstwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QFileInfo>
#include <QtMath>
#include <QStorageInfo>
#include <QDir>
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
    // Ensure parent pointers are valid throughout
    fixParentPointers(rootNode);
    // Precompute angles for entire tree and reset current root
    calculateNodeAngles(rootNode, 0.0, 360.0);
    // Revalidate current root by path if possible to avoid stale pointer
    if (!currentRootPath.isEmpty()) {
        SunburstNode* found = findByFullPath(rootNode, currentRootPath);
        currentRoot = found ? found : &rootNode;
    } else {
        currentRoot = &rootNode;
    }
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

void SunburstWidget::zoomToPath(const QString& path)
{
    // Normalize path separators to match how we stored fullPath
    QString norm = path; norm.replace('\\','/');
    // Recompute angles to ensure up-to-date geometry
    calculateNodeAngles(rootNode, 0.0, 360.0);
    SunburstNode* found = findByFullPath(rootNode, norm);
    if (found && found != currentRoot) {
        currentRoot = found;
        currentRootPath = norm;
        calculateNodeAngles(*currentRoot, 0.0, 360.0);
        update();
    }
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
    
    // Draw navigation (breadcrumbs + header) first so user always has a way back
    drawBreadcrumbs(painter);
    drawHeaderInfo(painter);

    // Draw sunburst
    drawSunburst(painter);
    
    // Draw labels
    drawLabels(painter);
}

void SunburstWidget::mousePressEvent(QMouseEvent* event)
{
    // Handle breadcrumb clicks and reset first
    for (const auto &pair : breadcrumbHit) {
        if (pair.second.contains(event->pos())) {
            QString targetPath = pair.first; // normalized path stored
            SunburstNode* found = findByFullPath(rootNode, targetPath);
            currentRoot = found ? found : &rootNode;
            currentRootPath = found ? targetPath : QString();
            calculateNodeAngles(*currentRoot, 0.0, 360.0);
            update();
            return;
        }
    }
    if (event->button() == Qt::LeftButton) {
        SunburstNode* node = findNodeAt(event->pos());
        if (node && node != currentRoot) {
            if (!node->children.empty()) {
                currentRoot = node;
                currentRootPath = QString(currentRoot->fullPath).replace('\\','/');
                calculateNodeAngles(*currentRoot, 0.0, 360.0);
                update();
            }
            return;
        }
    } else if (event->button() == Qt::RightButton) {
        if (currentRoot && currentRoot->parent) {
            currentRoot = currentRoot->parent;
            currentRootPath = currentRoot->fullPath.isEmpty()? rootPath : QString(currentRoot->fullPath).replace('\\','/');
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

QString SunburstWidget::normalizePath(const QString& p) const
{
    QString n = p;
    n.replace('\\','/');
    return n;
}

void SunburstWidget::buildSunburstTree(const std::vector<ScannerWrapper::DirectoryInfo>& directories)
{
    // Build a multi-level hierarchy relative to rootPath
    rootNode = SunburstNode();
    rootNode.name = "Root";
    rootNode.fullPath = "";
    rootNode.size = 0;
    rootNode.depth = 0;

    QString base = normalizePath(rootPath);
    for (const auto& d : directories) {
        rootNode.size += d.size;
        QString p = normalizePath(d.path);
        if (!base.isEmpty() && p.startsWith(base)) p = p.mid(base.length());
        QStringList parts = p.split('/', Qt::SkipEmptyParts);
        addPath(rootNode, parts, 0, d.size, normalizePath(d.path), base);
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
    
    // Calculate center and radius; reserve space for breadcrumbs + header
    QRectF widgetRect = rect();
    QFontMetrics fmCrumb(QFont("Arial", 9, QFont::Bold));
    QFontMetrics fmHead(QFont("Arial", 10));
    double topUi = 10 + (fmCrumb.height() + 6) + 6 + (fmHead.height() + 8) + 8;
    QRectF chartRect = widgetRect.adjusted(0, topUi, 0, 0);
    center = chartRect.center() + viewOffset;
    double padding = 20.0;
    double maxRadius = qMin(chartRect.width(), chartRect.height()) / 2.0 - padding;
    
    // Compute inner hub radius now; draw hub after wedges to ensure visibility
    double innerRadius = qMax(24.0, maxRadius * 0.18); // keep inner hub proportional
    
    // Precompute angles for current root
    calculateNodeAngles(*currentRoot, 0.0, 360.0);

    // Determine ring count from depth
    int maxD = getMaxDepth(*currentRoot) - currentRoot->depth;
    maxD = qBound(1, maxD, 6);
    double ringWidth = (maxRadius - innerRadius) / maxD;

    // Draw rings depth by depth
    std::function<void(const SunburstNode&)> drawDepth = [&](const SunburstNode& node) {
        // Sort children by size (descending) to give larger slices drawing priority and clearer label placement
        std::vector<const SunburstNode*> ordered;
        ordered.reserve(node.children.size());
        for (const auto &c : node.children) ordered.push_back(&c);
        std::sort(ordered.begin(), ordered.end(), [](const SunburstNode* a, const SunburstNode* b){ return a->size > b->size; });

        for (const SunburstNode* chp : ordered) {
            const SunburstNode &ch = *chp;
            int depthIndex = ch.depth - currentRoot->depth; // 1..maxD
            if (depthIndex > 0 && depthIndex <= maxD) {
                double rInner = innerRadius + ringWidth * (depthIndex - 1);
                double rOuter = rInner + ringWidth;
                QRectF rect(center.x() - rOuter, center.y() - rOuter, rOuter*2, rOuter*2);
                painter.setBrush(ch.color);
                painter.setPen(QPen(Qt::black, 1));
                if (ch.spanAngle > 0.0)
                    painter.drawPie(rect, int(ch.startAngle * 16), int(ch.spanAngle * 16));
            }
            if (!ch.children.empty()) drawDepth(ch);
        }
    };
    drawDepth(*currentRoot);

    // Draw center circle and text on top
    painter.setBrush(QColor(53, 53, 53));
    painter.setPen(QPen(Qt::white, 1));
    painter.drawEllipse(center, innerRadius, innerRadius);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    // Always show PHYSICAL usage (for the volume containing rootPath)
    QString centerText;
    QStorageInfo si(rootPath.isEmpty() ? QDir::rootPath() : rootPath);
    if (si.isValid() && si.isReady() && si.bytesTotal() > 0) {
        quint64 physicalUsed = si.bytesTotal() - si.bytesFree();
        centerText = formatSize(physicalUsed);
    } else {
        centerText = formatSize(currentRoot->size); // fallback
    }
    painter.drawText(QRectF(center.x() - innerRadius, center.y() - 10, innerRadius * 2, 20),
                     Qt::AlignCenter, centerText);
}

std::vector<QString> SunburstWidget::buildBreadcrumbPaths() const
{
    std::vector<QString> paths;
    const SunburstNode* n = currentRoot;
    while (n) {
        QString p = (n == &rootNode) ? normalizePath(rootPath) : normalizePath(n->fullPath);
        paths.push_back(p);
        n = n->parent;
    }
    std::reverse(paths.begin(), paths.end());
    return paths;
}

void SunburstWidget::drawBreadcrumbs(QPainter& painter)
{
    breadcrumbHit.clear();
    auto crumbs = buildBreadcrumbPaths();
    if (crumbs.empty()) return;

    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.setPen(Qt::white);
    QFontMetrics fm(painter.font());

    int x = 10;
    int y = 10;
    for (size_t i = 0; i < crumbs.size(); ++i) {
        QString label;
        if (i == 0) {
            label = QString("Root");
        } else {
            QStringList parts = crumbs[i].split('/', Qt::SkipEmptyParts);
            label = parts.isEmpty() ? QString("?") : parts.last();
        }
        QString shown = fm.elidedText(label, Qt::ElideRight, 140);
        QRectF r(x, y, fm.horizontalAdvance(shown) + 10, fm.height() + 6);
        painter.setBrush(QColor(60,60,60));
        painter.setPen(QPen(QColor(90,90,90)));
        painter.drawRoundedRect(r, 4, 4);
        painter.setPen(Qt::white);
        painter.drawText(r.adjusted(5,0, -5,0), Qt::AlignVCenter|Qt::AlignLeft, shown);
        breadcrumbHit.push_back(qMakePair(crumbs[i], r));
        x += r.width() + 8;
        if (i+1 < crumbs.size()) {
            painter.drawText(x, y + r.height()/2 + fm.ascent()/2 - 4, ">>");
            x += 14;
        }
        if (x > width() - 100) break;
    }
}

int SunburstWidget::computeDirCount(const SunburstNode& node) const
{
    int count = 1;
    for (const auto& ch : node.children) count += computeDirCount(ch);
    return count;
}

void SunburstWidget::drawHeaderInfo(QPainter& painter)
{
    painter.setFont(QFont("Arial", 10));
    painter.setPen(Qt::white);
    QFontMetrics fm(painter.font());

    QString path = (currentRoot == &rootNode) ? rootPath : currentRoot->fullPath;
    QString sizeStr = formatSize(currentRoot->size);
    int dirCount = computeDirCount(*currentRoot) - 1;

    QString info = QString("%1    •    Logical size: %2    •    %3 dirs")
                    .arg(fm.elidedText(path, Qt::ElideMiddle, width() - 260))
                    .arg(sizeStr)
                    .arg(dirCount);

    QRectF bar(10, 34, width() - 20, fm.height() + 8);
    painter.setBrush(QColor(45,45,45));
    painter.setPen(QPen(QColor(70,70,70)));
    painter.drawRoundedRect(bar, 4, 4);
    painter.setPen(Qt::white);
    painter.drawText(bar.adjusted(8,0,-8,0), Qt::AlignVCenter|Qt::AlignLeft, info);
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
    QFontMetrics fmCrumb(QFont("Arial", 9, QFont::Bold));
    QFontMetrics fmHead(QFont("Arial", 10));
    double topUi = 10 + (fmCrumb.height() + 6) + 6 + (fmHead.height() + 8) + 8;
    QRectF chartRect = widgetRect.adjusted(0, topUi, 0, 0);
    center = chartRect.center() + viewOffset;
    double padding = 20.0;
    double maxRadius = qMin(chartRect.width(), chartRect.height()) / 2.0 - padding;
    double innerRadius = qMax(24.0, maxRadius * 0.18);
    int maxD = qBound(1, getMaxDepth(*currentRoot) - currentRoot->depth, 6);
    double ringWidth = (maxRadius - innerRadius) / maxD;

    std::function<void(const SunburstNode&)> drawText = [&](const SunburstNode& node) {
        // Larger slices first for clearer label placement
        std::vector<const SunburstNode*> ordered;
        ordered.reserve(node.children.size());
        for (const auto &c : node.children) ordered.push_back(&c);
        std::sort(ordered.begin(), ordered.end(), [](const SunburstNode* a, const SunburstNode* b){ return a->size > b->size; });

        for (const SunburstNode* chp : ordered) {
            const SunburstNode &ch = *chp;
            int depthIndex = ch.depth - currentRoot->depth; // 1..maxD
            if (depthIndex > 0 && depthIndex <= maxD) {
                if (ch.spanAngle > 8.0) {
                    double mid = ch.startAngle + ch.spanAngle/2.0;
                    double radians = qDegreesToRadians(mid);
                    double radius = innerRadius + ringWidth * (depthIndex-0.45);
                    QPointF labelPos(center.x() + radius * qCos(radians),
                                     center.y() + radius * qSin(radians));
                    QString label = ch.name;
                    if (label.length() > 18) label = label.left(15) + "...";
                    QRectF box(labelPos.x() - 60, labelPos.y() - 10, 120, 20);
                    painter.drawText(box, Qt::AlignCenter, label);
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
    // Convert point to angle and radius relative to center with reserved top UI
    QFontMetrics fmCrumb(QFont("Arial", 9, QFont::Bold));
    QFontMetrics fmHead(QFont("Arial", 10));
    double topUi = 10 + (fmCrumb.height() + 6) + 6 + (fmHead.height() + 8) + 8;
    QRectF chartRect = QRectF(QPointF(0,0), QSizeF(width(), height())).adjusted(0, topUi, 0, 0);
    QPointF chartCenter = chartRect.center() + viewOffset;
    QPointF v = point - chartCenter;
    double r = std::hypot(v.x(), v.y());
    double ang = std::atan2(v.y(), v.x());
    double deg = qRadiansToDegrees(ang);
    if (deg < 0) deg += 360.0;

    double padding = 20.0;
    double maxRadius = qMin(chartRect.width(), chartRect.height()) / 2.0 - padding;
    double innerRadius = qMax(24.0, maxRadius * 0.18);
    int maxD = qBound(1, getMaxDepth(*currentRoot) - currentRoot->depth, 6);
    double ringWidth = (maxRadius - innerRadius) / maxD;
    if (r < innerRadius || r > innerRadius + ringWidth * maxD || ringWidth <= 0.0) {
        return currentRoot; // clicked outside valid rings; treat as current root
    }
    int ringIdx = int((r - innerRadius) / ringWidth) + 1; // 1..maxD

    SunburstNode* result = nullptr;
    std::function<void(SunburstNode&)> find = [&](SunburstNode& node){
        for (auto &ch : node.children) {
            int dIdx = ch.depth - currentRoot->depth;
            if (dIdx == ringIdx) {
                if (ch.spanAngle <= 0.001) { continue; }
                // Inclusive of start, exclusive of end to avoid tiny-gap ambiguity
                if (deg >= ch.startAngle && deg < ch.startAngle + ch.spanAngle) {
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

SunburstWidget::SunburstNode* SunburstWidget::findByFullPath(SunburstNode& node, const QString& normalizedPath)
{
    if (!node.fullPath.isEmpty()) {
        QString n = node.fullPath; n.replace('\\','/');
        if (n == normalizedPath) return &node;
    }
    for (auto &ch : node.children) {
        SunburstNode* f = findByFullPath(ch, normalizedPath);
        if (f) return f;
    }
    return nullptr;
}

void SunburstWidget::updateLayout()
{
    // Update layout when widget is resized
    ensureCurrentRootValid();
    update();
}

void SunburstWidget::ensureCurrentRootValid()
{
    if (!currentRoot) { currentRoot = &rootNode; currentRootPath.clear(); return; }
    // If currentRoot pointer is from previous tree, try to re-find by stored path
    if (currentRoot != &rootNode && !currentRootPath.isEmpty()) {
        SunburstNode* found = findByFullPath(rootNode, currentRootPath);
        if (!found) {
            currentRoot = &rootNode; // fallback
            currentRootPath.clear();
        } else {
            currentRoot = found;
        }
    }
}

void SunburstWidget::fixParentPointers(SunburstNode& node)
{
    for (auto &ch : node.children) {
        ch.parent = &node;
        fixParentPointers(ch);
    }
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

void SunburstWidget::addPath(SunburstNode& root, const QStringList& parts, int idx, uint64_t size, const QString& leafFullPath, const QString& accumFullPath)
{
    if (idx >= parts.size()) return;
    const QString &part = parts[idx];

    // Find or create child
    for (auto &ch : root.children) {
        if (ch.name == part) {
            ch.size += size;
            QString nextAccum = accumFullPath.isEmpty() ? part : (accumFullPath + "/" + part);
            addPath(ch, parts, idx+1, size, leafFullPath, nextAccum);
            return;
        }
    }
    SunburstNode child;
    child.name = part;
    QString nextAccum = accumFullPath.isEmpty() ? part : (accumFullPath + "/" + part);
    child.fullPath = nextAccum;
    child.size = size;
    child.depth = root.depth + 1;
    child.parent = &root;
    setNodeColor(child);
    root.children.push_back(child);
    // Recurse deeper
    addPath(root.children.back(), parts, idx+1, size, leafFullPath, nextAccum);
}
