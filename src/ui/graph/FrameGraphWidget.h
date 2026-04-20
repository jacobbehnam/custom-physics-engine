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

        Count, // Must be last (just dont have to hard code the count elsewhere)
    };

    explicit FrameGraphWidget(QWidget* parent = nullptr);

    void setSnapshots(const std::vector<ObjectSnapshot>& snapshots);
    void clear();
    void setMetric(Metric metric);
    void setSelectorVisible(bool visible);
    QString metricLabel() const;
    static QString metricLabel(Metric metric);

private:
    QComboBox* metricSelector;
    QLabel* titleLabel;
    QWidget* canvas;
    Metric currentMetric;
};
