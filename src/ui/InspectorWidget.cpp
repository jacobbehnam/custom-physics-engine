#include "InspectorWidget.h"

#include <QComboBox>
#include <QStackedWidget>

#include "graphics/core/SceneObject.h"

#include <glm/glm.hpp>
#include <qgroupbox.h>
#include <QLineEdit>
#include <qpushbutton.h>
#include <qslider.h>
#include <QCheckBox>

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
    transformRows.emplace_back("Position", this)
        .addVec3(
            [obj]()->glm::vec3{ return obj->getPosition(); },
            [obj](glm::vec3 v){ obj->setPosition(v); }
        );
    transformRows.emplace_back("Scale", this)
        .addVec3(
            [obj]()->glm::vec3{ return obj->getScale(); },
            [obj](glm::vec3 v) { obj->setScale(v); }
        );
    // TODO: this works good enough for 2D but for 3D the conversion between euler angles and quaternions is wonky
    transformRows.emplace_back("Rotation", this)
        .addVec3(
            [obj]()->glm::vec3{ return glm::degrees(obj->getRotation()); },
            [obj](glm::vec3 v) { obj->setRotation(glm::radians(v)); }
        );

    if (Physics::PhysicsBody* body = obj->getPhysicsBody()) {
        QCheckBox* unknownV0Ck = nullptr;
        Vector3Widget* velocityVec = nullptr;
        transformRows.emplace_back("Velocity", this)
            .addCheckbox(
                [body]()->bool{ return body->isUnknown("v0", BodyLock::NOLOCK); },
                [body](bool b) { body->setUnknown("v0", b, BodyLock::NOLOCK); },
                [&unknownV0Ck](QCheckBox* cb) {
                    unknownV0Ck = cb;
                    cb->setToolTip("Unknown?");
                }
            )
            .addVec3(
                [body]()->glm::vec3{ return body->getVelocity(BodyLock::NOLOCK); },
                [body](glm::vec3 v){ body->setVelocity(v, BodyLock::NOLOCK); },
                [&velocityVec](Vector3Widget* vec) {
                    velocityVec = vec;
                }
            );
        // Disable velocity input if specified as unknown
        if (unknownV0Ck && velocityVec) {
            connect(unknownV0Ck, &QCheckBox::toggled, [velocityVec](bool checked) {
                velocityVec->setEnabled(!checked);
            });
            velocityVec->setEnabled(!unknownV0Ck->isChecked());
        }
        transformRows.emplace_back("Mass", this)
            .addScalar(
                [body]()->float{ return body->getMass(BodyLock::NOLOCK); },
                [body](float newMass){ body->setMass(newMass, BodyLock::NOLOCK); }
            );
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
            forceRows.emplace_back(QString::fromStdString(name), this)
                .addVec3(
                    [body, name](){ return body->getForce(name, BodyLock::NOLOCK); },
                    nullptr
                );
        } else {
            forceRows.emplace_back(QString::fromStdString(name), this)
                .addVec3(
                    [body, name](){ return body->getForce(name, BodyLock::NOLOCK); },
                    [body, name](glm::vec3 v){ body->setForce(name, v, BodyLock::NOLOCK); }
                );
        }
        auto& row = forceRows.back();
        layout->addRow(row.getLabel(), row.getEditor());
    }

    forceRows.emplace_back("Net Force", this)
        .addVec3(
            [body](){
                glm::vec3 sum{0.0f};
                for (auto const& [n, f] : body->getAllForces(BodyLock::NOLOCK)) sum += f;
                return sum;
            },
            nullptr
        );
    auto& row = forceRows.back();
    layout->addRow(row.getLabel(), row.getEditor());
}

void InspectorWidget::populateGlobals(QVBoxLayout *layout) {
    QGroupBox* globalsGroup = new QGroupBox("Globals");
    auto* formLayout = new QFormLayout(globalsGroup);
    layout->addWidget(globalsGroup);
    globalsRows.emplace_back("Gravitational\nAcceleration", this)
        .addVec3(
            [this]()->glm::vec3{ return sceneManager->getGlobalAcceleration(); },
            [this](glm::vec3 a){ sceneManager->setGlobalAcceleration(a); }
        );
    globalsRows.emplace_back("Sim Speed", this)
        .addScalar(
            [this]()->float{ return sceneManager->getSimSpeed(); },
            [this](float s){ sceneManager->setSimSpeed(s); }
        );

    // Stop condition
    QWidget* logicContainer = new QWidget();
    logicContainer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    logicContainer->setMinimumWidth(1);

    QVBoxLayout* mainContainerLayout = new QVBoxLayout(logicContainer);
    mainContainerLayout->setContentsMargins(0, 0, 0, 0);
    mainContainerLayout->setSpacing(2);

    QWidget* equationRow = new QWidget();
    QHBoxLayout* equationLayout = new QHBoxLayout(equationRow);
    equationLayout->setContentsMargins(0, 0, 0, 0);

    QComboBox* subjectCombo = new QComboBox();
    subjectCombo->addItem("-- Subject --", -1);
    for (auto* obj : sceneManager->getObjects()) {
        subjectCombo->addItem(QString::number(obj->getObjectID()), obj->getObjectID());
    }

    QComboBox* propCombo = new QComboBox();
    propCombo->addItem("Pos Y", 0);
    propCombo->addItem("Vel Y", 1);
    propCombo->addItem("Dist to Obj", 2);
    propCombo->addItem("Dist to Point", 3);

    QComboBox* opCombo = new QComboBox();
    opCombo->addItem("<", 0);
    opCombo->addItem(">", 1);

    ScalarWidget* thresholdWidget = new ScalarWidget();
    connect(thresholdWidget, &ScalarWidget::valueChanged, [this](double v){
        sceneManager->stopCondition.value = (float)v;
    });

    equationLayout->addWidget(subjectCombo);
    equationLayout->addWidget(propCombo);
    equationLayout->addWidget(opCombo);
    equationLayout->addWidget(thresholdWidget);
    equationLayout->addStretch();

    QStackedWidget* parameterStack = new QStackedWidget();
    parameterStack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    QWidget* emptyPage = new QWidget();
    emptyPage->setFixedHeight(0);
    parameterStack->addWidget(emptyPage);

    QComboBox* targetObjCombo = new QComboBox();
    parameterStack->addWidget(targetObjCombo);

    Vector3Widget* targetVecWidget = new Vector3Widget();
    parameterStack->addWidget(targetVecWidget);

    mainContainerLayout->addWidget(equationRow);
    mainContainerLayout->addWidget(parameterStack);

    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, subjectCombo](int){
        sceneManager->stopCondition.subjectID = subjectCombo->currentData().toInt();
    });
    connect(propCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx){
        sceneManager->stopCondition.property = idx;
    });
    connect(opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx){
        sceneManager->stopCondition.op = idx;
    });
    connect(targetObjCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, targetObjCombo](int){
        sceneManager->stopCondition.targetID = targetObjCombo->currentData().toInt();
    });
    connect(targetVecWidget, &Vector3Widget::valueChanged, [this](glm::vec3 v){
        sceneManager->stopCondition.targetPos = v;
    });

    auto updateUiState = [=]() {
        int subjIdx = subjectCombo->findData(sceneManager->stopCondition.subjectID);
        subjectCombo->setCurrentIndex(subjIdx != -1 ? subjIdx : 0);

        propCombo->setCurrentIndex(sceneManager->stopCondition.property);
        opCombo->setCurrentIndex(sceneManager->stopCondition.op);

        thresholdWidget->blockSignals(true);
        thresholdWidget->setValue(sceneManager->stopCondition.value);
        thresholdWidget->blockSignals(false);

        targetObjCombo->clear();
        for (auto* obj : sceneManager->getObjects()) {
            targetObjCombo->addItem(QString::number(obj->getObjectID()), obj->getObjectID());
        }
        int tIdx = targetObjCombo->findData(sceneManager->stopCondition.targetID);
        if (tIdx != -1) targetObjCombo->setCurrentIndex(tIdx);

        targetVecWidget->blockSignals(true);
        targetVecWidget->setValue(sceneManager->stopCondition.targetPos);
        targetVecWidget->blockSignals(false);

        int prop = sceneManager->stopCondition.property;
        if (prop == 2) {
            parameterStack->setCurrentIndex(1);
            parameterStack->setVisible(true);
        }
        else if (prop == 3) {
            parameterStack->setCurrentIndex(2);
            parameterStack->setVisible(true);
        }
        else {
            parameterStack->setCurrentIndex(0);
            parameterStack->setVisible(false);
        }

        bool active = (sceneManager->stopCondition.subjectID != -1);
        propCombo->setEnabled(active);
        opCombo->setEnabled(active);
        thresholdWidget->setEnabled(active);
        parameterStack->setEnabled(active);
    };

    updateUiState();

    connect(propCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int){
        updateUiState();
    });
    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int){
        updateUiState();
    });

    globalsRows.emplace_back("Stop Condition", logicContainer, updateUiState);

    QWidget* buttonContainer = new QWidget();
    QHBoxLayout* btnLayout = new QHBoxLayout(buttonContainer);
    btnLayout->setContentsMargins(0,0,0,0);
    QPushButton* solveBtn = new QPushButton("SOLVE PHYSICS");
    solveBtn->setFixedHeight(30);
    connect(solveBtn, &QPushButton::clicked, this, &InspectorWidget::runSolver);
    btnLayout->addWidget(solveBtn);
    globalsRows.emplace_back("", buttonContainer, nullptr);

    QWidget* timeRow = new QWidget();
    QHBoxLayout* timeLayout = new QHBoxLayout(timeRow);
    timeLayout->setContentsMargins(0,0,0,0);

    ScalarWidget* timeWidget = new ScalarWidget();
    timeWidget->setValue(0.0); // 0.0 means "Don't enforce time"
    timeLayout->addWidget(timeWidget);

    // Store in member variable so we can read it later
    this->timeConstraintWidget = timeWidget;

    globalsRows.emplace_back("Flight Time (s):", timeRow, nullptr);

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

    if (currentObject->getPhysicsBody())
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

    if (!body->isUnknown("r0", BodyLock::LOCK)) {
        glm::vec3 r0 = body->getPosition(BodyLock::LOCK);
        knowns["r0_x"] = r0.x;
        knowns["r0_y"] = r0.y;
        knowns["r0_z"] = r0.z;
    }

    if (!body->isUnknown("v0", BodyLock::LOCK)) {
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

    bool isInverse = body->isUnknown("v0", BodyLock::NOLOCK) || body->isUnknown("r0", BodyLock::NOLOCK);

    std::string unknownKey = "Event";
    if (body->isUnknown("v0", BodyLock::LOCK)) {
        unknownKey = "v0";
    }
    else if (body->isUnknown("r0", BodyLock::LOCK)) {
        unknownKey = "r0";
    }

    double targetTime = -1.0;
    if (body->isUnknown("v0", BodyLock::LOCK)) {
        targetTime = timeConstraintWidget->getValue();
        if (targetTime <= 0.0001) targetTime = -1.0;
    }

    knowns["Target_Time"] = targetTime;

    sceneManager->physicsSystem->solveProblem(body, knowns, unknownKey);
    refresh();
}
