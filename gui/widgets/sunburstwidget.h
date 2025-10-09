#ifndef SUNBURSTWIDGET_H
#define SUNBURSTWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QColor>
#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <vector>
#include <cstdint>

#include "../scanner_wrapper.h"

class SunburstWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SunburstWidget(QWidget *parent = nullptr);
    ~SunburstWidget();

    void updateData(const std::vector<ScannerWrapper::DirectoryInfo>& directories, uint64_t totalSize);
    void setRootPath(const QString& path);
    void resetView();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    struct SunburstNode {
        QString name;
        QString fullPath;
        uint64_t size;
        QRectF rect;
        QColor color;
        std::vector<SunburstNode> children;
        SunburstNode* parent;
        int depth;
        double startAngle; // degrees within current root
        double spanAngle;  // degrees within current root
        
        SunburstNode() : size(0), parent(nullptr), depth(0), startAngle(0.0), spanAngle(0.0) {}
    };
    
    SunburstNode rootNode;
    SunburstNode* currentRoot;
    QString rootPath;
    QPointF center;
    double scale;
    int currentDepth;
    int maxDepth;
    QPoint mousePos;
    bool isDragging;
    QPointF dragStart;
    QPointF viewOffset;
    
    // Colors for different file types (fallback) and vivid top-level palette
    std::vector<QColor> fileTypeColors;
    std::vector<QColor> vividPalette;
    
    void buildSunburstTree(const std::vector<ScannerWrapper::DirectoryInfo>& directories);
    void drawSunburst(QPainter& painter);
    void drawNode(QPainter& painter, const SunburstNode& node, double startAngle, double spanAngle);
    void drawLabels(QPainter& painter);
    QColor getFileTypeColor(const QString& path) const;
    QString formatSize(uint64_t bytes) const;
    SunburstNode* findNodeAt(const QPointF& point);
    void updateLayout();
    void calculateNodeAngles(SunburstNode& node, double startAngle, double spanAngle);
    void setNodeColor(SunburstNode& node);
    void addPath(SunburstNode& root, const QStringList& parts, int idx, uint64_t size, const QString& fullPath);
    int getMaxDepth(const SunburstNode& node) const;
    
    // Animation
    QTimer* animationTimer;
    double animationProgress;
    bool isAnimating;
    
private slots:
    void updateAnimation();
};

#endif // SUNBURSTWIDGET_H
