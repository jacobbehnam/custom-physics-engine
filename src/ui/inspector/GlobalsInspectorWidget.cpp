#include "GlobalsInspectorWidget.h"
#include <QFormLayout>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QLabel>
#include <QGroupBox>

#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"
#include "ui/ScalarWidget.h"
#include "ui/Vector3Widget.h"

GlobalsInspectorWidget::GlobalsInspectorWidget(SceneManager* mgr, QWidget* parent) : IInspectorSection(parent), sceneManager(mgr) {
    layout = new QFormLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    setLayout(layout);

    createUiComponents();
}

void GlobalsInspectorWidget::createUiComponents() {
    {
        InspectorRow row("Gravity", this);
        row.addVec3(
            [this]() { return sceneManager->getGlobalAcceleration(); },
            [this](glm::vec3 g) { sceneManager->setGlobalAcceleration(g); },
            "m/s²"
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }

    {
        InspectorRow row("Sim Speed", this);
        row.addScalar(
            [this]() { return sceneManager->getSimSpeed(); },
            [this](float s) { sceneManager->setSimSpeed(s); },
            "x"
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }

    createStopConditionUi();

    // {
    //     timeConstraint = new ScalarWidget(" s");
    //     timeConstraint->setValue(0.0);
    //
    //     InspectorRow row("Flight Time", timeConstraint, nullptr);
    //     layout->addRow(row.getLabel(), row.getEditor());
    //     rows.push_back(std::move(row));
    // }

    {
        QPushButton* solveBtn = new QPushButton("SOLVE PHYSICS");
        solveBtn->setFixedHeight(30);

        connect(solveBtn, &QPushButton::clicked, this, &GlobalsInspectorWidget::runSolver);

        InspectorRow row("", solveBtn, nullptr);
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
}

void GlobalsInspectorWidget::createStopConditionUi() {
    QWidget* logicContainer = new QWidget();
    logicContainer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    QVBoxLayout* mainContainerLayout = new QVBoxLayout(logicContainer);
    mainContainerLayout->setContentsMargins(0, 0, 0, 0);
    mainContainerLayout->setSpacing(2);

    QWidget* equationRow = new QWidget();
    QHBoxLayout* equationLayout = new QHBoxLayout(equationRow);
    equationLayout->setContentsMargins(0, 0, 0, 0);

    QComboBox* subjectCombo = new QComboBox();
    QComboBox* propCombo = new QComboBox();
    QComboBox* opCombo = new QComboBox();
    ScalarWidget* thresholdWidget = new ScalarWidget(" m");

    QStackedWidget* parameterStack = new QStackedWidget();
    parameterStack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    QWidget* emptyPage = new QWidget();
    emptyPage->setFixedHeight(0);
    parameterStack->addWidget(emptyPage);

    QComboBox* targetObjCombo = new QComboBox();
    parameterStack->addWidget(targetObjCombo);

    Vector3Widget* targetVecWidget = new Vector3Widget();
    parameterStack->addWidget(targetVecWidget);

    subjectCombo->addItem("-- Subject --", -1);
    for (auto* obj : sceneManager->getObjects()) {
        subjectCombo->addItem(QString::fromStdString(obj->getName()), obj->getObjectID());
    }
    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, subjectCombo](int){
        sceneManager->stopCondition.subjectID = subjectCombo->currentData().toInt();
        refresh();
    });

    propCombo->addItem("Pos Y", 0);
    propCombo->addItem("Vel Y", 1);
    propCombo->addItem("Dist to Obj", 2);
    propCombo->addItem("Dist to Point", 3);
    connect(propCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx){
        sceneManager->stopCondition.property = idx;
        refresh();
    });

    opCombo->addItem("<", 0);
    opCombo->addItem(">", 1);
    connect(opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx){
        sceneManager->stopCondition.op = idx;
    });

    connect(thresholdWidget, &ScalarWidget::valueChanged, [this](double v){
        sceneManager->stopCondition.value = (float)v;
    });

    connect(targetObjCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, targetObjCombo](int){
        sceneManager->stopCondition.targetID = targetObjCombo->currentData().toInt();
    });

    connect(targetVecWidget, &Vector3Widget::valueChanged, [this](glm::vec3 v){
        sceneManager->stopCondition.targetPos = v;
    });

    equationLayout->addWidget(subjectCombo);
    equationLayout->addWidget(propCombo);
    equationLayout->addWidget(opCombo);
    equationLayout->addWidget(thresholdWidget);

    mainContainerLayout->addWidget(equationRow);
    mainContainerLayout->addWidget(parameterStack);

    auto updateLogic = [=]() {
        int subjIdx = subjectCombo->findData(sceneManager->stopCondition.subjectID);
        const QSignalBlocker b1(subjectCombo);
        subjectCombo->setCurrentIndex(subjIdx != -1 ? subjIdx : 0);

        if (subjectCombo->count() != sceneManager->getObjects().size() + 1) {
             subjectCombo->clear();
             subjectCombo->addItem("-- Subject --", -1);
             for (auto* obj : sceneManager->getObjects()) {
                 subjectCombo->addItem(QString::fromStdString(obj->getName()), obj->getObjectID());
             }
             subjIdx = subjectCombo->findData(sceneManager->stopCondition.subjectID);
             subjectCombo->setCurrentIndex(subjIdx != -1 ? subjIdx : 0);
        }

        const QSignalBlocker b2(propCombo);
        propCombo->setCurrentIndex(sceneManager->stopCondition.property);

        const QSignalBlocker b3(opCombo);
        opCombo->setCurrentIndex(sceneManager->stopCondition.op);

        const QSignalBlocker b4(thresholdWidget);
        thresholdWidget->setValue(sceneManager->stopCondition.value);

        int prop = sceneManager->stopCondition.property;
        if (prop == 2) { // Dist to Obj
            parameterStack->setCurrentIndex(1);
            parameterStack->setVisible(true);

            if (targetObjCombo->count() != sceneManager->getObjects().size()) {
                const QSignalBlocker b(targetObjCombo);
                targetObjCombo->clear();
                for (auto* obj : sceneManager->getObjects()) {
                    targetObjCombo->addItem(QString::fromStdString(obj->getName()), obj->getObjectID());
                }
            }

            int tIdx = targetObjCombo->findData(sceneManager->stopCondition.targetID);
            if (tIdx != -1 && targetObjCombo->currentIndex() != tIdx) {
                const QSignalBlocker b(targetObjCombo);
                targetObjCombo->setCurrentIndex(tIdx);
            }
        }
        else if (prop == 3) { // Dist to Point
            parameterStack->setCurrentIndex(2);
            parameterStack->setVisible(true);

            const QSignalBlocker b5(targetVecWidget);
            targetVecWidget->setValue(sceneManager->stopCondition.targetPos);
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

    updateLogic();

    InspectorRow row("Stop Condition", logicContainer, updateLogic);
    layout->addRow(row.getLabel(), row.getEditor());
    rows.push_back(std::move(row));
}

void GlobalsInspectorWidget::load(SceneObject* object) {
    setVisible(true);
    refresh();
}

void GlobalsInspectorWidget::unload() {
    // Globals persist
}

void GlobalsInspectorWidget::refresh() {
    for (auto& row : rows)
        row.refresh();
}

void GlobalsInspectorWidget::runSolver() {
    auto& cond = sceneManager->stopCondition;
    if (cond.subjectID == -1) return;

    Physics::PhysicsBody* body = sceneManager->physicsSystem->getBodyById(cond.subjectID);
    if (!body) return;

    std::unordered_map<std::string, double> knowns;

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
    if (timeConstraint) {
        targetTime = timeConstraint->getValue();
        if (targetTime <= 0.0001) targetTime = -1.0;
    }
    knowns["Target_Time"] = targetTime;

    SceneObject* visualSubject = nullptr;
    for (auto* obj : sceneManager->getObjects()) {
        if (static_cast<int>(obj->getObjectID()) == cond.subjectID) {
            visualSubject = obj;
            break;
        }
    }

    if (visualSubject) {
        sceneManager->setCameraTarget(visualSubject);
    }

    sceneManager->physicsSystem->solveProblem(body, knowns, unknownKey);
    emit solveRequested();
    refresh();
}