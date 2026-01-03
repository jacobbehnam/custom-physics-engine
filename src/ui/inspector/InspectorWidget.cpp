#include "InspectorWidget.h"

#include <QVBoxLayout>

#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"
#include "physics/PhysicsSystem.h"
#include "physics/PhysicsBody.h"

#include "ui/ScalarWidget.h"
#include "ui/inspector/TransformInspectorWidget.h"
#include "ui/inspector/PhysicsInspectorWidget.h"
#include "ui/inspector/ForcesInspectorWidget.h"
#include "ui/inspector/GlobalsInspectorWidget.h"

InspectorWidget::InspectorWidget(SceneManager* sceneMgr, QWidget* parent) : QWidget(parent), sceneManager(sceneMgr) {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 0, 15, 0);
    mainLayout->setSpacing(6);

    setMinimumWidth(350);

    transformWidget = new TransformInspectorWidget(this);
    physicsWidget = new PhysicsInspectorWidget(this);
    forcesWidget = new ForcesInspectorWidget(this);
    globalsWidget = new GlobalsInspectorWidget(sceneMgr, this);

    mainLayout->addWidget(transformWidget);
    mainLayout->addWidget(physicsWidget);
    mainLayout->addWidget(forcesWidget);
    mainLayout->addWidget(globalsWidget);
    mainLayout->addStretch();

    connect(globalsWidget, &GlobalsInspectorWidget::solveRequested, this, &InspectorWidget::refresh);

    refreshTimer.setInterval(100);
    connect(&refreshTimer, &QTimer::timeout, this, &InspectorWidget::refresh);
    refreshTimer.start();

    unloadObject();
}

void InspectorWidget::loadObject(SceneObject* obj) {
    currentObject = obj;

    transformWidget->load(obj);
    physicsWidget->load(obj);
    forcesWidget->load(obj);

    transformWidget->setVisible(true);
    physicsWidget->setVisible(obj && obj->getPhysicsBody());
    forcesWidget->setVisible(obj && obj->getPhysicsBody());

    globalsWidget->setVisible(false);
}

void InspectorWidget::unloadObject() {
    currentObject = nullptr;

    transformWidget->unload();
    physicsWidget->unload();
    forcesWidget->unload();

    transformWidget->setVisible(false);
    physicsWidget->setVisible(false);
    forcesWidget->setVisible(false);

    globalsWidget->setVisible(true);
}

void InspectorWidget::refresh() {
    if (!currentObject) {
        globalsWidget->refresh();
        return;
    }

    transformWidget->refresh();
    physicsWidget->refresh();
    forcesWidget->refresh();
}