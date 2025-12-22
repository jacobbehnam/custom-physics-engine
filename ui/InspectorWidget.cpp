#include "InspectorWidget.h"

#include <QComboBox>
#include <QStackedWidget>

#include "graphics/core/SceneObject.h"

#include <glm/glm.hpp>
#include <qgroupbox.h>
#include <QLineEdit>
#include <qpushbutton.h>
#include <qslider.h>

#include "InspectorRow.h"
#include "ScalarWidget.h"
#include "Vector3Widget.h"

InspectorWidget::InspectorWidget(SceneManager* sceneMgr, QWidget* parent) : QWidget(parent), sceneManager(sceneMgr) {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 0, 15, 0);

    setMinimumWidth(350);

    refreshTimer.setInterval(100);
    connect(&refreshTimer, &QTimer::timeout, this, &InspectorWidget::refresh);
    refreshTimer.start();

    unloadObject();
}

void InspectorWidget::loadObject(SceneObject* obj) {
    unloadObject(false);
    currentObject = obj;

    // Transform
    QGroupBox* transformGroup = new QGroupBox("Transform");
    auto* layout = new QFormLayout(transformGroup);
    mainLayout->addWidget(transformGroup);
    transformRows.emplace_back("Position",
        [obj]()->glm::vec3{ return obj->getPosition(); },
        [obj](glm::vec3 v){ obj->setPosition(v); },
        this);
    transformRows.emplace_back("Scale",
        [obj]()->glm::vec3{ return obj->getScale(); },
        [obj](glm::vec3 v) { obj->setScale(v); },
        this);
    // TODO: this works good enough for 2D but for 3D the conversion between euler angles and quaternions is wonky
    transformRows.emplace_back("Rotation",
        [obj]()->glm::vec3{ return glm::degrees(obj->getRotation()); },
        [obj](glm::vec3 v) { obj->setRotation(glm::radians(v)); },
        this);

    if (Physics::PhysicsBody* body = obj->getPhysicsBody()) {
        transformRows.emplace_back("Velocity",
            [body]()->glm::vec3{ return body->getVelocity(BodyLock::NOLOCK); },
            [body](glm::vec3 v){ body->setVelocity(v, BodyLock::NOLOCK); },
            this);
        transformRows.emplace_back("Mass",
            [body]()->float{ return body->getMass(BodyLock::NOLOCK); },
            [body](float newMass){ body->setMass(newMass, BodyLock::NOLOCK); },
            this);
    }
    for (InspectorRow row : transformRows) {
        layout->addRow(row.getLabel(), row.getEditor());
    }

    // Forces
    QGroupBox* forcesGroup = new QGroupBox("Forces");
    auto* layout2 = new QVBoxLayout(forcesGroup);
    mainLayout->addWidget(forcesGroup);

    auto* addForceWidget = new QWidget(forcesGroup);
    auto* addForceLayout = new QHBoxLayout(addForceWidget);
    auto* addForceField = new QLineEdit("Add a force", addForceWidget);
    auto* addForceButton = new QPushButton(addForceWidget);

    addForceLayout->addWidget(addForceField);
    addForceLayout->addWidget(addForceButton);
    layout2->addWidget(addForceWidget);

    auto* forcesWidget = new QWidget(forcesGroup);
    auto* forcesLayout = new QFormLayout(forcesWidget);

    if (Physics::PhysicsBody* body = obj->getPhysicsBody()) {
        populateForces(body, forcesLayout);
        connect(addForceButton, &QPushButton::clicked, this, [=]() {
            std::string text = addForceField->text().toStdString();
            if (text.empty()) return;
            if (std::isalnum(text.front()))
                text[0] = static_cast<char>(std::toupper(text[0]));
            body->setForce(text, glm::vec3(0.0f), BodyLock::NOLOCK);
            populateForces(body, forcesLayout);
        });
    }
    layout2->addWidget(forcesWidget);
}

void InspectorWidget::populateForces(Physics::PhysicsBody* body, QFormLayout *layout) {
    clearLayout(layout);
    forceRows.clear();

    std::map<std::string, glm::vec3> forcesCopy = body->getAllForces(BodyLock::NOLOCK);
    for (auto const& [name, vec] : forcesCopy) {
        if (name == "Gravity" || name == "Normal") {
            forceRows.emplace_back(
              QString::fromStdString(name),
              [body, name](){ return body->getForce(name, BodyLock::NOLOCK); }
            );
        } else {
            forceRows.emplace_back(
              QString::fromStdString(name),
              [body, name](){ return body->getForce(name, BodyLock::NOLOCK); },
              [body, name](glm::vec3 v){ body->setForce(name, v, BodyLock::NOLOCK); },
              this
            );
        }
        auto& row = forceRows.back();
        layout->addRow(row.getLabel(), row.getEditor());
    }

    forceRows.emplace_back(
      "Net Force",
      [body](){
        glm::vec3 sum{0.0f};
        for (auto const& [n, f] : body->getAllForces(BodyLock::NOLOCK)) sum += f;
        return sum;
      }
    );
    auto& row = forceRows.back();
    layout->addRow(row.getLabel(), row.getEditor());
}

void InspectorWidget::populateGlobals(QVBoxLayout *layout) {
    QGroupBox* globalsGroup = new QGroupBox("Globals");
    auto* formLayout = new QFormLayout(globalsGroup);
    layout->addWidget(globalsGroup);
    globalsRows.emplace_back("Gravitational \n Acceleration",
        [this]()->glm::vec3{ return sceneManager->getGlobalAcceleration(); },
        [this](glm::vec3 a){ sceneManager->setGlobalAcceleration(a); },
        this);
    globalsRows.emplace_back("Sim Speed",
        [this]()->float{ return sceneManager->getSimSpeed(); },
        [this](float s){ sceneManager->setSimSpeed(s); },
        this);

    // Stop condition
    QWidget* logicContainer = new QWidget();
    QHBoxLayout* logicLayout = new QHBoxLayout(logicContainer);
    logicLayout->setContentsMargins(0,0,0,0);

    QComboBox* subjectCombo = new QComboBox();
    subjectCombo->addItem("-- Subject --", -1);
    for (auto* obj : sceneManager->getObjects()) {
        subjectCombo->addItem(QString::number(obj->getObjectID()), obj->getObjectID());
    }

    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, subjectCombo](int){
        sceneManager->stopCondition.subjectID = subjectCombo->currentData().toInt();
    });

    QComboBox* propCombo = new QComboBox();
    propCombo->addItem("Pos Y", 0);
    propCombo->addItem("Vel Y", 1);
    propCombo->addItem("Dist Obj", 2);
    propCombo->addItem("Dist Pnt", 3);

    connect(propCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx){
        sceneManager->stopCondition.property = idx;
    });

    QComboBox* opCombo = new QComboBox();
    opCombo->addItem("<", 0);
    opCombo->addItem(">", 1);

    connect(opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx){
        sceneManager->stopCondition.op = idx;
    });

    QStackedWidget* targetStack = new QStackedWidget();

    ScalarWidget* valWidget = new ScalarWidget();
    connect(valWidget, &ScalarWidget::valueChanged, [this](double v){
        sceneManager->stopCondition.value = (float)v;
    });
    targetStack->addWidget(valWidget);

    QComboBox* targetObjCombo = new QComboBox();
    targetStack->addWidget(targetObjCombo);

    connect(targetObjCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, targetObjCombo](int){
        sceneManager->stopCondition.targetID = targetObjCombo->currentData().toInt();
    });

    Vector3Widget* targetVecWidget = new Vector3Widget();
    connect(targetVecWidget, &Vector3Widget::valueChanged, [this](glm::vec3 v){
        sceneManager->stopCondition.targetPos = v;
    });
    targetStack->addWidget(targetVecWidget);

    logicLayout->addWidget(subjectCombo);
    logicLayout->addWidget(propCombo);
    logicLayout->addWidget(opCombo);
    logicLayout->addWidget(targetStack);

    auto stopConditionUpdater = [=]() {
        int subjIdx = subjectCombo->findData(sceneManager->stopCondition.subjectID);
        if (subjIdx != -1) subjectCombo->setCurrentIndex(subjIdx);
        else subjectCombo->setCurrentIndex(0);

        propCombo->setCurrentIndex(sceneManager->stopCondition.property);
        opCombo->setCurrentIndex(sceneManager->stopCondition.op);

        int prop = sceneManager->stopCondition.property;
        if (prop == 2) targetStack->setCurrentIndex(1);
        else if (prop == 3) targetStack->setCurrentIndex(2);
        else targetStack->setCurrentIndex(0);

        valWidget->setValue(sceneManager->stopCondition.value);
        targetVecWidget->setValue(sceneManager->stopCondition.targetPos);

        targetObjCombo->clear();
        for (auto* obj : sceneManager->getObjects()) {
            targetObjCombo->addItem(QString::number(obj->getObjectID()), obj->getObjectID());
        }
        int tIdx = targetObjCombo->findData(sceneManager->stopCondition.targetID);
        if (tIdx != -1) targetObjCombo->setCurrentIndex(tIdx);

        bool active = (sceneManager->stopCondition.subjectID != -1);
        propCombo->setEnabled(active);
        opCombo->setEnabled(active);
        targetStack->setEnabled(active);
    };

    stopConditionUpdater();

    connect(propCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx){
        if (idx == 2) targetStack->setCurrentIndex(1);
        else if (idx == 3) targetStack->setCurrentIndex(2);
        else targetStack->setCurrentIndex(0);
    });

    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int){
         bool active = (subjectCombo->currentData().toInt() != -1);
         propCombo->setEnabled(active);
         opCombo->setEnabled(active);
         targetStack->setEnabled(active);
    });

    globalsRows.emplace_back("Stop Condition", logicContainer, stopConditionUpdater);

    // Solve button
    QWidget* buttonContainer = new QWidget();
    QHBoxLayout* btnLayout = new QHBoxLayout(buttonContainer);
    btnLayout->setContentsMargins(0,0,0,0);

    QPushButton* solveBtn = new QPushButton("SOLVE PHYSICS");
    solveBtn->setFixedHeight(30);

    connect(solveBtn, &QPushButton::clicked, this, &InspectorWidget::runSolver);
    btnLayout->addWidget(solveBtn);
    globalsRows.emplace_back("", buttonContainer, nullptr);

    for (auto& row : globalsRows) {
        formLayout->addRow(row.getLabel(), row.getEditor());
    }
}

void InspectorWidget::unloadObject(bool loadGlobals) {
    clearLayout(mainLayout);
    transformRows.clear();
    forceRows.clear();
    globalsRows.clear();
    currentObject = nullptr;
    if (loadGlobals)
        populateGlobals(mainLayout);
}

void InspectorWidget::clearLayout(QVBoxLayout* layout) {
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item; // We are responsible for managing memory when we take a QLayoutItem
    }
}

void InspectorWidget::clearLayout(QFormLayout* layout) {
    while (layout->rowCount() > 0) {
        QLayoutItem* labelItem = layout->itemAt(0, QFormLayout::LabelRole);
        QLayoutItem* fieldItem = layout->itemAt(0, QFormLayout::FieldRole);

        if (labelItem && labelItem->widget()) {
            labelItem->widget()->deleteLater();
        }
        if (fieldItem && fieldItem->widget()) {
            fieldItem->widget()->deleteLater();
        }

        layout->removeRow(0);
    }
}

void InspectorWidget::refresh() {
    if (!currentObject) return;

    std::unique_lock<std::mutex> guard = currentObject->getPhysicsBody()->lockState();

    for (InspectorRow &row : transformRows) {
        row.update();
    }
    for (InspectorRow &row : forceRows) {
        row.update();
    }
}

void InspectorWidget::runSolver() {
    std::unordered_map<std::string, double> knowns;
    auto& cond = sceneManager->stopCondition;
    Physics::PhysicsBody* body = sceneManager->physicsSystem->getBodyById(cond.subjectID);

    if (!body->isUnknown("r0")) {
        glm::vec3 r0 = body->getPosition(BodyLock::LOCK);
        knowns["r0_x"] = r0.x;
        knowns["r0_y"] = r0.y;
        knowns["r0_z"] = r0.z;
    }

    if (!body->isUnknown("v0")) {
        glm::vec3 v0 = body->getVelocity(BodyLock::LOCK);
        knowns["v0_x"] = v0.x;
        knowns["v0_y"] = v0.y;
        knowns["v0_z"] = v0.z;
    }

    knowns["Stop_SubjectID"] = (double)cond.subjectID;
    knowns["Stop_Prop"] = (double)cond.property;
    knowns["Stop_Op"] = (double)cond.op;
    knowns["Stop_Val"] = (double)cond.value;
    knowns["Stop_TargetID"] = (double)cond.targetID;

    knowns["Stop_Val_X"] = cond.targetPos.x;
    knowns["Stop_Val_Y"] = cond.targetPos.y;
    knowns["Stop_Val_Z"] = cond.targetPos.z;

    bool isInverse = body->isUnknown("v0") || body->isUnknown("r0");

    if (isInverse) {
        if (cond.property == 3) {
            knowns["Constraint_Pos_X"] = cond.targetPos.x;
            knowns["Constraint_Pos_Y"] = cond.targetPos.y;
            knowns["Constraint_Pos_Z"] = cond.targetPos.z;
        }
        else if (cond.property == 2) {
            // TODO: if target is a body
        }
    }

    std::string unknownKey = "Event";

    if (body->isUnknown("v0")) {
        unknownKey = "v0";
    }
    else if (body->isUnknown("r0")) {
        unknownKey = "r0";
    }

    sceneManager->physicsSystem->solveProblem(body, knowns, unknownKey);

    refresh();
}
