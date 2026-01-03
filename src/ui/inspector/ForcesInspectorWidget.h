#pragma once
#include <vector>
#include <QFormLayout>
#include "IInspectorSection.h"
#include "InspectorRow.h"

class SceneObject;
namespace Physics { class PhysicsBody; }

class ForcesInspectorWidget : public IInspectorSection {
    Q_OBJECT
public:
    explicit ForcesInspectorWidget(QWidget* parent = nullptr);

    void load(SceneObject* object) override;
    void unload() override;
    void refresh() override;

private:
    SceneObject* selectedObject = nullptr;
    QFormLayout* layout;
    std::vector<InspectorRow> rows;

    Physics::PhysicsBody* getBody() const;

    void clearRows();
};