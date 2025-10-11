#ifndef TREEMAPWIDGET_H
#define TREEMAPWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QColor>
#include <QFont>
#include <QPoint>
#include <QRect>
#include <QString>
#include <vector>
#include <cstdint>

#include "../scanner_wrapper.h"

class TreemapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TreemapWidget(QWidget *parent = nullptr);
    ~TreemapWidget();

    void updateData(const std::vector<ScannerWrapper::DirectoryInfo>& directories, uint64_t totalSize);
    void setRootPath(const QString& path);
    void resetView();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct TreemapNode {
        QString name;
        QString fullPath;
        uint64_t size;
        QRect rect;
        QColor color;
        std::vector<TreemapNode> children;
        int depth;
        bool isVisible;
        TreemapNode* parent;
        
        TreemapNode() : size(0), depth(0), isVisible(true), parent(nullptr) {}
    };
    
    TreemapNode rootNode;
    TreemapNode* hoveredNode;
    TreemapNode* selectedNode;
    TreemapNode* currentRoot;
    QPoint mousePos;
    bool isDragging;
    QPoint dragStart;
    QPointF viewOffset;
    double scale;
    int currentDepth;
    int maxDepth;
    QString rootPath;
    QString currentRootPath;
    int legendHeight;
    bool colorByType;
    QRect modeToggleRect;
    
    // Colors for different file types
    std::vector<QColor> fileTypeColors;
    std::vector<QColor> vividPalette; // base colors per top-level child
    std::vector<Qt::BrushStyle> patternStyles; // hatch patterns for contrast
    
    void buildTreemapTree(const std::vector<ScannerWrapper::DirectoryInfo>& directories);
    void addPath(TreemapNode& parent, const QStringList& parts, int index, uint64_t size, const QString& fullPath);
    void fixParentPointers(TreemapNode& node);
    void drawTreemap(QPainter& painter);
    void drawNode(QPainter& painter, const TreemapNode& node, int depthLimit = 4);
    void drawLabels(QPainter& painter);
    void drawLegend(QPainter& painter);
    void drawBreadcrumbs(QPainter& painter);
    QColor getFileTypeColor(const QString& path) const;
    QColor getNodeColor(const TreemapNode& node) const; // current mode color
    QColor getContrastingTextColor(const QColor& bg) const; // dark on bright, light on dark
    QString formatSize(uint64_t bytes) const;
    TreemapNode* findNodeAt(const QPoint& point);
    TreemapNode* findNodeAtRecursive(TreemapNode& node, const QPoint& point);
    TreemapNode* findByFullPath(TreemapNode& node, const QString& path);
    void updateLayout();
    void squarifyTreemap(TreemapNode& node, const QRect& rect);
    void setNodeColor(TreemapNode& node);
    void calculateNodeRects(TreemapNode& node, const QRect& rect);
    QString normalizePath(const QString& p) const;
    void ensureCurrentRootValid();
    void showTooltipForNode(TreemapNode* node, const QPoint& globalPos);
    void assignColors();
    Qt::BrushStyle getPatternForNode(const TreemapNode& node) const;
    std::vector<QString> buildBreadcrumbPaths() const;
    QVector<QPair<QString, QRectF>> breadcrumbHit;
    
    // Animation
    QTimer* animationTimer;
    double animationProgress;
    bool isAnimating;
    
private slots:
    void updateAnimation();
};

#endif // TREEMAPWIDGET_H
