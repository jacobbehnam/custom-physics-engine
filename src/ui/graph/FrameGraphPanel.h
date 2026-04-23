#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QScrollArea>
#include <array>
#include <vector>

#include "Metric.h"
#include "physics/PhysicsBody.h"

class FrameGraphWidget;

class FrameGraphPanel : public QWidget {
    Q_OBJECT
public:
    explicit FrameGraphPanel(QWidget* parent = nullptr);
    void loadSnapshots(const std::vector<ObjectSnapshot>& snapshots);
    void clear();
protected:
    void resizeEvent(QResizeEvent* event) override; 
private:
    void recomputeTimeAndValueRanges();
    void relayoutGraphs();
    QGridLayout* gridLayout;
    QScrollArea* scrollArea;
    std::vector<FrameGraphWidget*> frameGraphs;
    int currentColumns = 0;
    std::vector<ObjectSnapshot> m_snapshots;
    float m_tMin = 0.0f;
    float m_tMax = 0.0f;
    std::array<std::pair<float, float>, kPlottableMetricCount> m_valueMinMaxPerMetric{};
};
