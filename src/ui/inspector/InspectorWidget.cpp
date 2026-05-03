#include "InspectorWidget.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"
#include "physics/PhysicsSystem.h"
#include "physics/PhysicsBody.h"

#include "ui/ScalarWidget.h"
#include "ui/inspector/TransformInspectorWidget.h"
#include "ui/inspector/PhysicsInspectorWidget.h"
#include "ui/inspector/ThermalInspectorWidget.h"
#include "ui/inspector/ForcesInspectorWidget.h"
#include "ui/inspector/GlobalsInspectorWidget.h"

InspectorWidget::InspectorWidget(SceneManager* sceneMgr, QWidget* parent) : QWidget(parent), sceneManager(sceneMgr) {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 0, 5, 0);
    mainLayout->setSpacing(6);

    setMinimumWidth(350);

    inspectorTabs = new QTabWidget(this);

    transformWidget = new TransformInspectorWidget(this);
    physicsWidget = new PhysicsInspectorWidget(this);
    thermalWidget = new ThermalInspectorWidget(this);
    forcesWidget = new ForcesInspectorWidget(this);
    globalsWidget = new GlobalsInspectorWidget(sceneMgr, this);

    auto* objectTab = new QWidget(inspectorTabs);
    auto* objectLayout = new QVBoxLayout(objectTab);
    objectLayout->setContentsMargins(6, 6, 6, 6);
    objectLayout->setSpacing(6);
    objectLayout->addWidget(transformWidget);
    objectLayout->addWidget(physicsWidget);
    objectLayout->addWidget(forcesWidget);
    objectLayout->addStretch();

    auto* thermalTab = new QWidget(inspectorTabs);
    auto* thermalLayout = new QVBoxLayout(thermalTab);
    thermalLayout->setContentsMargins(6, 6, 6, 6);
    thermalLayout->setSpacing(6);
    thermalLayout->addWidget(thermalWidget);
    thermalLayout->addStretch();

    auto* sceneTab = new QWidget(inspectorTabs);
    auto* sceneLayout = new QVBoxLayout(sceneTab);
    sceneLayout->setContentsMargins(6, 6, 6, 6);
    sceneLayout->setSpacing(6);
    sceneLayout->addWidget(globalsWidget);
    sceneLayout->addStretch();

    objectTabIndex = inspectorTabs->addTab(objectTab, "Object");
    thermalTabIndex = inspectorTabs->addTab(thermalTab, "Thermal");
    sceneTabIndex = inspectorTabs->addTab(sceneTab, "Scene");

    mainLayout->addWidget(inspectorTabs);
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
    thermalWidget->load(obj);
    forcesWidget->load(obj);

    transformWidget->setVisible(true);
    physicsWidget->setVisible(obj && obj->getPhysicsBody());
    thermalWidget->setVisible(obj && obj->getPhysicsBody());
    forcesWidget->setVisible(obj && obj->getPhysicsBody());

    inspectorTabs->setTabEnabled(objectTabIndex, obj != nullptr);
    inspectorTabs->setTabEnabled(thermalTabIndex, obj && obj->getPhysicsBody());
    inspectorTabs->setCurrentIndex(objectTabIndex);
}

void InspectorWidget::unloadObject() {
    currentObject = nullptr;

    transformWidget->unload();
    physicsWidget->unload();
    thermalWidget->unload();
    forcesWidget->unload();

    transformWidget->setVisible(false);
    physicsWidget->setVisible(false);
    thermalWidget->setVisible(false);
    forcesWidget->setVisible(false);

    inspectorTabs->setTabEnabled(objectTabIndex, false);
    inspectorTabs->setTabEnabled(thermalTabIndex, false);
    inspectorTabs->setCurrentIndex(sceneTabIndex);
}

void InspectorWidget::refresh() {
    if (!currentObject) {
        globalsWidget->refresh();
        return;
    }

    transformWidget->refresh();
    physicsWidget->refresh();
    thermalWidget->refresh();
    forcesWidget->refresh();
    globalsWidget->refresh();
}
