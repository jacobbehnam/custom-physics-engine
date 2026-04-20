#include <QVBoxLayout>
#include <QResizeEvent>
#include <array>

#include "FrameGraphPanel.h"

namespace {
    constexpr int kMinimumCardWidth = 180;
}

FrameGraphPanel::FrameGraphPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto* container = new QWidget(scrollArea);
    gridLayout = new QGridLayout(container);
    gridLayout->setContentsMargins(8, 8, 8, 8);
    gridLayout->setSpacing(8);
    gridLayout->setAlignment(Qt::AlignTop);
    const std::array<FrameGraphWidget::Metric, 6> metrics = {
        FrameGraphWidget::Metric::PositionX, FrameGraphWidget::Metric::PositionY, FrameGraphWidget::Metric::PositionZ,
        FrameGraphWidget::Metric::VelocityX, FrameGraphWidget::Metric::VelocityY, FrameGraphWidget::Metric::VelocityZ
    };
    frameGraphs.reserve(metrics.size());
    for (auto metric : metrics) {
        auto* graph = new FrameGraphWidget(container);
        graph->setMetric(metric);
        graph->setSelectorVisible(false);
        frameGraphs.push_back(graph);
    }
    scrollArea->setWidget(container);
    layout->addWidget(scrollArea);
}

void FrameGraphPanel::loadSnapshots(const std::vector<ObjectSnapshot>& snapshots) {
    for (auto* graph : frameGraphs) {
        graph->setSnapshots(snapshots);
    }
}

void FrameGraphPanel::clear() {
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
