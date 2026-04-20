#include "FrameGraphWidget.h"

#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QStyle>
#include <QToolTip>
#include <QVBoxLayout>

#include "physics/PhysicsBody.h"
#include "FrameGraphCanvas.h"

namespace {
    constexpr int kLayoutMargin     = 6;
    constexpr int kLayoutSpacing    = 4;
    constexpr int kHeaderMargin     = 0;
    constexpr int kHeaderSpacing    = 4;
    constexpr int kMinWidgetHeight  = 150;
    constexpr int kMaxWidgetHeight  = 190;
}

FrameGraphWidget::FrameGraphWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(kLayoutMargin, kLayoutMargin, kLayoutMargin, kLayoutMargin);
    layout->setSpacing(kLayoutSpacing);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(kHeaderMargin, kHeaderMargin, kHeaderMargin, kHeaderMargin);
    headerLayout->setSpacing(kHeaderSpacing);

    titleLabel = new QLabel(metricLabel(), this);
    titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    metricSelector = new QComboBox(this);
    for (int i = 0; i < static_cast<int>(FrameGraphWidget::Metric::Count); ++i) {
        metricSelector->addItem(FrameGraphWidget::metricLabel(static_cast<Metric>(i)));
    }

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(metricSelector);

    canvas = new FrameGraphCanvas(this);

    layout->addLayout(headerLayout);
    layout->addWidget(canvas, 1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMinimumHeight(kMinWidgetHeight);
    setMaximumHeight(kMaxWidgetHeight);

    connect(metricSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
        setMetric(static_cast<Metric>(index));
    });
}

void FrameGraphWidget::setSnapshots(const std::vector<ObjectSnapshot>& snapshots) {
    static_cast<FrameGraphCanvas*>(canvas)->setSnapshots(snapshots);
}

void FrameGraphWidget::clear() {
    static_cast<FrameGraphCanvas*>(canvas)->clear();
}

void FrameGraphWidget::setMetric(Metric metric) {
    currentMetric = metric;
    titleLabel->setText(metricLabel());
    metricSelector->blockSignals(true);
    metricSelector->setCurrentIndex(static_cast<int>(metric));
    metricSelector->blockSignals(false);
    static_cast<FrameGraphCanvas*>(canvas)->setMetric(metric);
}

void FrameGraphWidget::setSelectorVisible(bool visible) {
    metricSelector->setVisible(visible);
}

QString FrameGraphWidget::metricLabel(Metric metric) {
    switch (metric) {
        case Metric::PositionX: return tr("Position X");
        case Metric::PositionY: return tr("Position Y");
        case Metric::PositionZ: return tr("Position Z");
        case Metric::VelocityX: return tr("Velocity X");
        case Metric::VelocityY: return tr("Velocity Y");
        case Metric::VelocityZ: return tr("Velocity Z");
        case Metric::Count:     return tr("Invalid Metric");
    }
    return tr("Unknown Metric");
}

QString FrameGraphWidget::metricLabel() const {
    return FrameGraphWidget::metricLabel(currentMetric);
}
