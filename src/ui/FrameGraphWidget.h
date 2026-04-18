#pragma once

#include <QWidget>
#include <vector>

class QComboBox;
class QLabel;

struct ObjectSnapshot;

class FrameGraphWidget : public QWidget {
    Q_OBJECT
public:
    enum class Metric {
        PositionX,
        PositionY,
        PositionZ,
        VelocityX,
        VelocityY,
        VelocityZ,
    };

    explicit FrameGraphWidget(QWidget* parent = nullptr);

    void setSnapshots(const std::vector<ObjectSnapshot>& snapshots);
    void clear();
    void setMetric(Metric metric);
    void setSelectorVisible(bool visible);
    QString metricLabel() const;

private:
    QComboBox* metricSelector;
    QLabel* titleLabel;
    QWidget* canvas;
    Metric currentMetric;
};
