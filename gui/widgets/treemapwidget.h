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
        
        TreemapNode() : size(0), depth(0), isVisible(true) {}
    };
    
    TreemapNode rootNode;
    TreemapNode* hoveredNode;
    TreemapNode* selectedNode;
    QPoint mousePos;
    bool isDragging;
    QPoint dragStart;
    QPointF viewOffset;
    double scale;
    int currentDepth;
    int maxDepth;
    
    // Colors for different file types
    std::vector<QColor> fileTypeColors;
    
    void buildTreemapTree(const std::vector<ScannerWrapper::DirectoryInfo>& directories);
    void drawTreemap(QPainter& painter);
    void drawNode(QPainter& painter, const TreemapNode& node);
    void drawLabels(QPainter& painter);
    QColor getFileTypeColor(const QString& path) const;
    QString formatSize(uint64_t bytes) const;
    TreemapNode* findNodeAt(const QPoint& point);
    void updateLayout();
    void squarifyTreemap(TreemapNode& node, const QRect& rect);
    void setNodeColor(TreemapNode& node);
    void calculateNodeRects(TreemapNode& node, const QRect& rect);
    
    // Animation
    QTimer* animationTimer;
    double animationProgress;
    bool isAnimating;
    
private slots:
    void updateAnimation();
};

#endif // TREEMAPWIDGET_H
