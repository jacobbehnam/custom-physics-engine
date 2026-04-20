#pragma once

#include <QWidget>
#include <vector>

#include "Metric.h"

class QComboBox;
class QLabel;
class FrameGraphCanvas;
struct ObjectSnapshot;

class FrameGraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit FrameGraphWidget(QWidget* parent = nullptr);

    void setSnapshots(const std::vector<ObjectSnapshot>& snapshots);
    void clear();
    void setMetric(Metric metric);
    void setSelectorVisible(bool visible);

private:
    QComboBox* metricSelector;
    QLabel* titleLabel;
    FrameGraphCanvas* canvas;
    Metric currentMetric = Metric::PositionX;
};
