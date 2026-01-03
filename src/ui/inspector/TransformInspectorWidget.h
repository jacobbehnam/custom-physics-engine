#pragma once

#include <vector>

#include "IInspectorSection.h"
#include "InspectorRow.h"

class SceneObject;
class QFormLayout;

class TransformInspectorWidget : public IInspectorSection {
    Q_OBJECT
public:
    explicit TransformInspectorWidget(QWidget* parent = nullptr);

    void load(SceneObject* object) override;
    void unload() override;
    void refresh() override;

private:
    SceneObject* selectedObject = nullptr;

    QFormLayout* layout;
    std::vector<InspectorRow> rows;

    void createUiComponents();
};
