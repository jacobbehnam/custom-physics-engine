#include <algorithm>
#include <cmath>
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
    constexpr int   kGridLines              = 3;

    int nearestIndexByX(const std::vector<QPointF>& pts, qreal mx) {
        if (pts.empty()) return -1;
        const auto it = std::lower_bound(pts.begin(), pts.end(), mx,
            [](const QPointF& p, qreal x) { return p.x() < x; });
        if (it == pts.begin()) {
            return 0;
        }
        if (it == pts.end()) {
            return static_cast<int>(pts.size() - 1);
        }
        const int i1 = static_cast<int>(it - pts.begin());
        const int i0 = i1 - 1;
        const qreal d0 = std::abs(pts[static_cast<size_t>(i0)].x() - mx);
        const qreal d1 = std::abs(pts[static_cast<size_t>(i1)].x() - mx);
        return d0 <= d1 ? i0 : i1;
    }
}

FrameGraphCanvas::FrameGraphCanvas(QWidget* parent) 
    : QWidget(parent) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(kMinCanvasHeight);
    resizeRebuildTimer.setSingleShot(true);
    resizeRebuildTimer.setInterval(120);
    connect(&resizeRebuildTimer, &QTimer::timeout, this, [this]() {
        rebuildPoints();
        invalidateCache();
        update();
    });
}

void FrameGraphCanvas::setSharedData(const std::vector<ObjectSnapshot>* frames,
    const std::array<std::pair<float, float>, kPlottableMetricCount>& valueMinMax, float tMinP, float tMaxP) {
    framesRef = (frames && !frames->empty()) ? frames : nullptr;
    if (framesRef) {
        valueMinMaxPerMetric = valueMinMax;
        tMin = tMinP;
        tMax = tMaxP;
    } else {
        valueMinMaxPerMetric = {};
        tMin = 0.0f;
        tMax = 0.0f;
    }
    rebuildPoints();
    invalidateCache();
    update();
}

void FrameGraphCanvas::clear() {
    framesRef = nullptr;
    valueMinMaxPerMetric = {};
    tMin = 0.0f;
    tMax = 0.0f;
    graphPoints.clear();
    graphPointFrameIndices.clear();
    hoverIndex = -1;
    invalidateCache();
    QToolTip::hideText();
    update();
}

void FrameGraphCanvas::setMetric(Metric metric) {
    currentMetric = metric;
    rebuildPoints();
    invalidateCache();
    update();
}

void FrameGraphCanvas::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    if (cacheDirty || baseCache.isNull()) {
        rebuildBaseCache();
    }

    QPainter painter(this);
    painter.drawPixmap(0, 0, baseCache);

    if (hoverIndex >= 0 && hoverIndex < static_cast<int>(graphPoints.size())) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPalette pal = palette();
        const QRect rect = plotRect();
        const QColor hoverColor = pal.color(QPalette::Highlight);
        const QPointF point = graphPoints[hoverIndex];
        painter.setPen(QPen(hoverColor, kHoverLineWidth, Qt::DashLine));
        painter.drawLine(QPointF(point.x(), rect.top()), QPointF(point.x(), rect.bottom()));
        painter.setBrush(hoverColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(point, kHoverPointRadius, kHoverPointRadius);
    }
}

void FrameGraphCanvas::rebuildBaseCache() {
    const qreal ratio = devicePixelRatioF();
    baseCache = QPixmap(size() * ratio);
    baseCache.setDevicePixelRatio(ratio);
    baseCache.fill(Qt::transparent);

    QPainter painter(&baseCache);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRect rect = plotRect();
    const QPalette pal = palette();
    const QColor panelColor = pal.color(QPalette::Window);
    const QColor plotColor = pal.color(QPalette::Base);
    const QColor borderColor = pal.color(QPalette::Mid);
    const QColor textColor = pal.color(QPalette::Text);
    const QColor mutedText = pal.color(QPalette::Midlight);
    const QColor lineColor = pal.color(QPalette::Highlight);
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
    if (graphPoints.empty() || !framesRef) {
        painter.setPen(mutedText);
        painter.drawText(rect, Qt::AlignCenter, tr("Select a simulated object to view its history."));
        cacheDirty = false;
        return;
    }

    QPainterPath path;
    path.moveTo(graphPoints.front());
    for (size_t i = 1; i < graphPoints.size(); ++i) {
        path.lineTo(graphPoints[i]);
    }
    painter.setPen(QPen(lineColor, kLineWidth));
    painter.drawPath(path);
    cacheDirty = false;
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
    const int nearestIndex = nearestIndexByX(graphPoints, event->position().x());
    if (nearestIndex < 0) return;
    if (nearestIndex == hoverIndex) return;

    hoverIndex = nearestIndex;
    const ObjectSnapshot& sample = (*framesRef)[graphPointFrameIndices[static_cast<size_t>(nearestIndex)]];
    const float value = objectSnapshotValue(currentMetric, sample);
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
    hoverIndex = -1;
    QToolTip::hideText();
    resizeRebuildTimer.start();
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
    graphPointFrameIndices.clear();
    hoverIndex = -1;
    if (!framesRef || framesRef->empty()) return;
    const QRect rect = plotRect();
    if (rect.width() <= 1 || rect.height() <= 1) return;

    const int m = static_cast<int>(currentMetric);
    if (m < 0 || m >= static_cast<int>(kPlottableMetricCount)) return;
    const float minTime = tMin;
    const float maxTime = tMax;
    const float minValue = valueMinMaxPerMetric[static_cast<size_t>(m)].first;
    const float maxValue = valueMinMaxPerMetric[static_cast<size_t>(m)].second;

    graphPoints.reserve(framesRef->size());
    graphPointFrameIndices.reserve(framesRef->size());
    const float invTime = maxTime > minTime ? 1.0f / (maxTime - minTime) : 0.0f;
    const float invValue = maxValue > minValue ? 1.0f / (maxValue - minValue) : 0.0f;

    for (size_t i = 0; i < framesRef->size(); ++i) {
        const auto& frame = (*framesRef)[i];
        const float timeAlpha = (frame.time - minTime) * invTime;
        const float valueAlpha = (objectSnapshotValue(currentMetric, frame) - minValue) * invValue;
        if (!std::isfinite(timeAlpha) || !std::isfinite(valueAlpha)) continue;

        const qreal x = rect.left() + static_cast<qreal>(timeAlpha) * rect.width();
        const qreal y = rect.bottom() - static_cast<qreal>(valueAlpha) * rect.height();
        if (!std::isfinite(x) || !std::isfinite(y)) continue;

        graphPoints.emplace_back(x, y);
        graphPointFrameIndices.push_back(i);
    }
}

void FrameGraphCanvas::invalidateCache() {
    cacheDirty = true;
}
