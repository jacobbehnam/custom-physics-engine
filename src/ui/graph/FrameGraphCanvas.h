#pragma once

#include <QWidget>
#include <array>
#include <vector>
#include <QPointF>
#include <QPixmap>
#include <QTimer>

#include "Metric.h"
#include "physics/PhysicsBody.h"

class FrameGraphCanvas : public QWidget {
    Q_OBJECT
public:
    explicit FrameGraphCanvas(QWidget* parent = nullptr);
    void setSharedData(const std::vector<ObjectSnapshot>* frames,
        const std::array<std::pair<float, float>, kPlottableMetricCount>& valueMinMax, float tMin, float tMax);
    void clear();
    void setMetric(Metric metric);
protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    int bottomLabelHeight() const;
    QRect plotRect() const;
    void rebuildPoints();
    void invalidateCache();
    void rebuildBaseCache();
    Metric currentMetric = Metric::PositionX;
    const std::vector<ObjectSnapshot>* framesRef = nullptr;
    std::array<std::pair<float, float>, kPlottableMetricCount> valueMinMaxPerMetric{};
    float tMin = 0.0f;
    float tMax = 0.0f;
    std::vector<QPointF> graphPoints;
    std::vector<size_t> graphPointFrameIndices;
    QPixmap baseCache;
    QTimer resizeRebuildTimer;
    bool cacheDirty = true;
    int hoverIndex = -1;
};
