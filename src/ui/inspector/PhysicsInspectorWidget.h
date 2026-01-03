#pragma once
#include <vector>
#include <QFormLayout>
#include "IInspectorSection.h"
#include "InspectorRow.h"

class SceneObject;
namespace Physics { class PhysicsBody; }

class PhysicsInspectorWidget : public IInspectorSection {
    Q_OBJECT
public:
    explicit PhysicsInspectorWidget(QWidget* parent = nullptr);

    void load(SceneObject* object) override;
    void unload() override;
    void refresh() override;

private:
    SceneObject* selectedObject = nullptr;
    QFormLayout* layout;
    std::vector<InspectorRow> rows;

    void createUiComponents();

    // Helper to safely get the body (returns nullptr if selectedObject is null)
    Physics::PhysicsBody* getBody() const;
};