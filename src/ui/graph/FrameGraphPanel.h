#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QScrollArea>
#include <vector>

#include "FrameGraphWidget.h"
#include "physics/PhysicsBody.h"

class FrameGraphPanel : public QWidget {
    Q_OBJECT
public:
    explicit FrameGraphPanel(QWidget* parent = nullptr);
    void loadSnapshots(const std::vector<ObjectSnapshot>& snapshots);
    void clear();
protected:
    void resizeEvent(QResizeEvent* event) override; 
private:
    void relayoutGraphs();
    QGridLayout* gridLayout;
    QScrollArea* scrollArea;
    std::vector<FrameGraphWidget*> frameGraphs;
    int currentColumns = 0;
};
