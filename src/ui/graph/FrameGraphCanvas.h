#pragma once

#include <QWidget>
#include <vector>
#include <QPointF>

#include "Metric.h"
#include "physics/PhysicsBody.h"

class FrameGraphCanvas : public QWidget {
    Q_OBJECT
public:
    explicit FrameGraphCanvas(QWidget* parent = nullptr);
    void setSnapshots(const std::vector<ObjectSnapshot>& snapshots);
    void clear();
    void setMetric(Metric metric);
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
    Metric currentMetric = Metric::PositionX;
    std::vector<ObjectSnapshot> frames;
    std::vector<QPointF> graphPoints;
    int hoverIndex = -1;
};
