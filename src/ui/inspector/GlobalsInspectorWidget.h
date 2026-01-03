#pragma once
#include <vector>
#include <QFormLayout>
#include "IInspectorSection.h"
#include "InspectorRow.h"

class SceneManager;
class ScalarWidget;

class GlobalsInspectorWidget : public IInspectorSection {
    Q_OBJECT
public:
    explicit GlobalsInspectorWidget(SceneManager* mgr, QWidget* parent = nullptr);

    void load(SceneObject* object) override;
    void unload() override;
    void refresh() override;

    // Specific accessors
    ScalarWidget* getTimeConstraintWidget() const { return timeConstraint; }

    signals:
        void solveRequested();

    public slots:
        void runSolver();

private:
    SceneManager* sceneManager;
    QFormLayout* layout;
    std::vector<InspectorRow> rows;

    ScalarWidget* timeConstraint = nullptr;

    void createUiComponents();
    void createStopConditionUi();
};