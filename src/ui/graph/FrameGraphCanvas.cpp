#include <algorithm>
#include <limits>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include "FrameGraphCanvas.h"

namespace {
    constexpr int   kMinCanvasHeight        = 110;
    constexpr float kLineWidth              = 2.0f;
    constexpr float kHoverLineWidth         = 1.0f;
    constexpr float kHoverPointRadius       = 4.0f;
    constexpr int   kPlotMarginLeft         = 8;
    constexpr int   kPlotMarginRight        = 8;
    constexpr int   kPlotMarginTop          = 6;
    constexpr int   kPlotMarginBottomOffset = 4;
    constexpr int   kLabelOffset            = 2;
    constexpr float kAxisPadding            = 0.5f;
    constexpr int   kGridLines              = 3;
}

FrameGraphCanvas::FrameGraphCanvas(QWidget* parent) 
    : QWidget(parent) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(kMinCanvasHeight);
}

void FrameGraphCanvas::setSnapshots(const std::vector<ObjectSnapshot>& snapshots) {
    frames = snapshots;
    rebuildPoints();
    update();
}

void FrameGraphCanvas::clear() {
    frames.clear();
    graphPoints.clear();
    hoverIndex = -1;
    QToolTip::hideText();
    update();
}

void FrameGraphCanvas::setMetric(Metric metric) {
    currentMetric = metric;
    rebuildPoints();
    update();
}

void FrameGraphCanvas::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QPalette pal = palette();
    const QRect rect = plotRect();
    const QColor panelColor = pal.color(QPalette::Window);
    const QColor plotColor = pal.color(QPalette::Base);
    const QColor borderColor = pal.color(QPalette::Mid);
    const QColor textColor = pal.color(QPalette::Text);
    const QColor mutedText = pal.color(QPalette::Midlight);
    const QColor lineColor = pal.color(QPalette::Highlight);
    const QColor hoverColor = pal.color(QPalette::Highlight);
    painter.fillRect(this->rect(), panelColor);
    painter.fillRect(rect, plotColor);
    painter.setPen(borderColor);
    painter.drawRect(rect);
    painter.setPen(mutedText);
    for (int i = 1; i < kGridLines; ++i) {
        const int y = rect.top() + (rect.height() * i) / kGridLines;
        painter.drawLine(rect.left(), y, rect.right(), y);
    }
    painter.setPen(textColor);
    painter.drawText(QRect(rect.left(), rect.bottom() + kLabelOffset, rect.width(), bottomLabelHeight()),
                     Qt::AlignRight | Qt::AlignVCenter,
                     tr("Time (s)"));
    if (graphPoints.empty()) {
        painter.setPen(mutedText);
        painter.drawText(rect, Qt::AlignCenter, tr("Select a simulated object to view its history."));
        return;
    }
    QPainterPath path;
    path.moveTo(graphPoints.front());
    for (size_t i = 1; i < graphPoints.size(); ++i) {
        path.lineTo(graphPoints[i]);
    }
    painter.setPen(QPen(lineColor, kLineWidth));
    painter.drawPath(path);
    if (hoverIndex >= 0 && hoverIndex < static_cast<int>(graphPoints.size())) {
        const QPointF point = graphPoints[hoverIndex];
        painter.setPen(QPen(hoverColor, kHoverLineWidth, Qt::DashLine));
        painter.drawLine(QPointF(point.x(), rect.top()), QPointF(point.x(), rect.bottom()));
        painter.setBrush(hoverColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(point, kHoverPointRadius, kHoverPointRadius);
    }
}

void FrameGraphCanvas::mouseMoveEvent(QMouseEvent* event) {
    const QRect rect = plotRect();
    if (graphPoints.empty() || !rect.contains(event->position().toPoint())) {
        if (hoverIndex != -1) {
            hoverIndex = -1;
            QToolTip::hideText();
            update();
        }
        return;
    }
    int nearestIndex = -1;
    qreal nearestDistance = std::numeric_limits<qreal>::max();
    for (int i = 0; i < static_cast<int>(graphPoints.size()); ++i) {
        const qreal distance = std::abs(graphPoints[i].x() - event->position().x());
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestIndex = i;
        }
    }
    if (nearestIndex == -1) return;
    hoverIndex = nearestIndex;
    const ObjectSnapshot& sample = frames[nearestIndex];
    const float value = metricValue(sample);
    QToolTip::showText(event->globalPosition().toPoint(),
                       tr("t=%1 s\n%2=%3")
                           .arg(sample.time, 0, 'f', 3)
                           .arg(metricLabel(currentMetric))
                           .arg(value, 0, 'f', 4),
                       this,
                       rect);
    update();
}

void FrameGraphCanvas::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    hoverIndex = -1;
    QToolTip::hideText();
    update();
}

void FrameGraphCanvas::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    rebuildPoints();
}

float FrameGraphCanvas::metricValue(const ObjectSnapshot& snapshot) const {
    switch (currentMetric) {
        case Metric::PositionX: return snapshot.position.x;
        case Metric::PositionY: return snapshot.position.y;
        case Metric::PositionZ: return snapshot.position.z;
        case Metric::VelocityX: return snapshot.velocity.x;
        case Metric::VelocityY: return snapshot.velocity.y;
        case Metric::VelocityZ: return snapshot.velocity.z;
        case Metric::Count:     return 0.0f;
    }
    return 0.0f;
}

int FrameGraphCanvas::bottomLabelHeight() const {
    return fontMetrics().height() + kLabelOffset;
}

QRect FrameGraphCanvas::plotRect() const {
    const int bottom = bottomLabelHeight() + kPlotMarginBottomOffset;
    return rect().adjusted(kPlotMarginLeft, kPlotMarginTop, -kPlotMarginRight, -bottom);
}

void FrameGraphCanvas::rebuildPoints() {
    graphPoints.clear();
    hoverIndex = -1;
    if (frames.empty()) return;
    const QRect rect = plotRect();
    if (rect.width() <= 1 || rect.height() <= 1) return;
    float minTime = frames.front().time;
    float maxTime = frames.back().time;
    float minValue = metricValue(frames.front());
    float maxValue = minValue;
    for (const auto& frame : frames) {
        minTime = std::min(minTime, frame.time);
        maxTime = std::max(maxTime, frame.time);
        const float value = metricValue(frame);
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }
    if (minTime == maxTime) {
        minTime -= kAxisPadding;
        maxTime += kAxisPadding;
    }
    if (minValue == maxValue) {
        minValue -= kAxisPadding;
        maxValue += kAxisPadding;
    }
    graphPoints.reserve(frames.size());
    for (const auto& frame : frames) {
        const float timeAlpha = (frame.time - minTime) / (maxTime - minTime);
        const float valueAlpha = (metricValue(frame) - minValue) / (maxValue - minValue);
        const qreal x = rect.left() + timeAlpha * rect.width();
        const qreal y = rect.bottom() - valueAlpha * rect.height();
        graphPoints.emplace_back(x, y);
    }
}
