#pragma once

#include <QWidget>
#include <vector>
#include <QPointF>

#include "FrameGraphWidget.h"
#include "physics/PhysicsBody.h"

class FrameGraphCanvas : public QWidget {
    Q_OBJECT
public:
    explicit FrameGraphCanvas(FrameGraphWidget* owner);
    void setSnapshots(const std::vector<ObjectSnapshot>& snapshots);
    void clear();
    void setMetric(FrameGraphWidget::Metric metric);
protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    float metricValue(const ObjectSnapshot& snapshot) const;
    int bottomLabelHeight() const;
    QRect plotRect() const;
    void rebuildPoints();
    FrameGraphWidget* graphWidget;
    FrameGraphWidget::Metric currentMetric = FrameGraphWidget::Metric::PositionX;
    std::vector<ObjectSnapshot> frames;
    std::vector<QPointF> graphPoints;
    int hoverIndex = -1;
};
