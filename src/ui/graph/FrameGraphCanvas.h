#pragma once

#include <QWidget>
#include <array>
#include <atomic>
#include <memory>
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
    void setSharedData(std::shared_ptr<const std::vector<ObjectSnapshot>> frames,
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
    void requestPointRebuild();
    void invalidateCache();
    void rebuildBaseCache();
    void applyRebuiltPoints(uint64_t generation, std::vector<QPointF> points, std::vector<size_t> frameIndices);
    Metric currentMetric = Metric::PositionX;
    std::shared_ptr<const std::vector<ObjectSnapshot>> framesData;
    std::array<std::pair<float, float>, kPlottableMetricCount> valueMinMaxPerMetric{};
    float tMin = 0.0f;
    float tMax = 0.0f;
    std::vector<QPointF> graphPoints;
    std::vector<size_t> graphPointFrameIndices;
    QPixmap baseCache;
    QTimer resizeRebuildTimer;
    std::atomic<uint64_t> rebuildGeneration{0};
    bool cacheDirty = true;
    int hoverIndex = -1;
};
