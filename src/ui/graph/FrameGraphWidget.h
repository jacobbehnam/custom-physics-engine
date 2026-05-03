#pragma once

#include <QWidget>
#include <array>
#include <memory>
#include <vector>

#include "Metric.h"

class QComboBox;
class QLabel;
class FrameGraphCanvas;

class FrameGraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit FrameGraphWidget(QWidget* parent = nullptr);

    void setSharedData(std::shared_ptr<const std::vector<ObjectSnapshot>> frames,
        const std::array<std::pair<float, float>, kPlottableMetricCount>& valueMinMax, float tMin, float tMax);
    void clear();
    void setMetric(Metric metric);
    void setSelectorVisible(bool visible);

private:
    QComboBox* metricSelector;
    QLabel* titleLabel;
    FrameGraphCanvas* canvas;
    Metric currentMetric = Metric::PositionX;
};
