#include <algorithm>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <array>

#include "FrameGraphPanel.h"
#include "FrameGraphWidget.h"
#include "Metric.h"

namespace {
    constexpr int   kMinimumCardWidth = 180;
    constexpr int   kLayoutMargin     = 0;
    constexpr int   kGridMargin       = 8;
    constexpr int   kGridSpacing      = 8;
    constexpr float kAxisPadding      = 0.5f;
}

FrameGraphPanel::FrameGraphPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(kLayoutMargin, kLayoutMargin, kLayoutMargin, kLayoutMargin);
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto* container = new QWidget(scrollArea);
    gridLayout = new QGridLayout(container);
    gridLayout->setContentsMargins(kGridMargin, kGridMargin, kGridMargin, kGridMargin);
    gridLayout->setSpacing(kGridSpacing);
    gridLayout->setAlignment(Qt::AlignTop);

    int cntMetric = static_cast<int>(Metric::Count);
    frameGraphs.reserve(cntMetric);
    for (int i = 0; i < cntMetric; ++i) {
        Metric metric = static_cast<Metric>(i);
        auto* graph = new FrameGraphWidget(container);
        graph->setMetric(metric);
        graph->setSelectorVisible(false);
        frameGraphs.push_back(graph);
    }
    scrollArea->setWidget(container);
    layout->addWidget(scrollArea);
}

void FrameGraphPanel::recomputeTimeAndValueRanges() {
    if (m_snapshots.empty()) {
        m_tMin = m_tMax = 0.0f;
        m_valueMinMaxPerMetric = {};
        return;
    }
    m_tMin = m_tMax = m_snapshots.front().time;
    for (int m = 0; m < static_cast<int>(kPlottableMetricCount); ++m) {
        const float v = objectSnapshotValue(static_cast<Metric>(m), m_snapshots.front());
        m_valueMinMaxPerMetric[static_cast<size_t>(m)] = {v, v};
    }
    for (const auto& s : m_snapshots) {
        m_tMin = std::min(m_tMin, s.time);
        m_tMax = std::max(m_tMax, s.time);
        for (int m = 0; m < static_cast<int>(kPlottableMetricCount); ++m) {
            const float v = objectSnapshotValue(static_cast<Metric>(m), s);
            auto& pr = m_valueMinMaxPerMetric[static_cast<size_t>(m)];
            pr.first = std::min(pr.first, v);
            pr.second = std::max(pr.second, v);
        }
    }
    if (m_tMin == m_tMax) {
        m_tMin -= kAxisPadding;
        m_tMax += kAxisPadding;
    }
    for (auto& pr : m_valueMinMaxPerMetric) {
        if (pr.first == pr.second) {
            pr.first -= kAxisPadding;
            pr.second += kAxisPadding;
        }
    }
}

void FrameGraphPanel::loadSnapshots(const std::vector<ObjectSnapshot>& snapshots) {
    m_snapshots = snapshots;
    recomputeTimeAndValueRanges();
    if (m_snapshots.empty()) {
        for (auto* graph : frameGraphs) {
            graph->clear();
        }
        return;
    }
    for (auto* graph : frameGraphs) {
        graph->setSharedData(&m_snapshots, m_valueMinMaxPerMetric, m_tMin, m_tMax);
    }
}

void FrameGraphPanel::clear() {
    m_snapshots.clear();
    m_tMin = m_tMax = 0.0f;
    m_valueMinMaxPerMetric = {};
    for (auto* graph : frameGraphs) {
        graph->clear();
    }
}

void FrameGraphPanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    relayoutGraphs();
}

void FrameGraphPanel::relayoutGraphs() {
    if (!gridLayout) return;
    const int availableWidth = scrollArea->viewport()->width() - gridLayout->contentsMargins().left() - gridLayout->contentsMargins().right();
    const int spacing = gridLayout->horizontalSpacing();
    const int columns = std::max(1, (availableWidth + spacing) / (kMinimumCardWidth + spacing));
    if (columns == currentColumns && gridLayout->count() == static_cast<int>(frameGraphs.size())) {
        return;
    }
    currentColumns = columns;
    for (auto* graph : frameGraphs) {
        gridLayout->removeWidget(graph);
    }
    const int maxTracks = static_cast<int>(frameGraphs.size());
    for (int index = 0; index < maxTracks; ++index) {
        gridLayout->setColumnStretch(index, 0);
        gridLayout->setColumnMinimumWidth(index, 0);
        gridLayout->setRowStretch(index, 0);
        gridLayout->setRowMinimumHeight(index, 0);
    }
    for (int i = 0; i < maxTracks; ++i) {
        const int row = i / columns;
        const int column = i % columns;
        gridLayout->addWidget(frameGraphs[i], row, column);
    }
    for (int column = 0; column < columns; ++column) {
        gridLayout->setColumnStretch(column, 1);
    }
    gridLayout->invalidate();
    scrollArea->widget()->updateGeometry();
}
