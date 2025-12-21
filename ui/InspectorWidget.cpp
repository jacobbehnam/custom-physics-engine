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

    for (InspectorRow row : globalsRows) {
        formLayout->addRow(row.getLabel(), row.getEditor());
    }

    // Stop conditions:
    QGroupBox* stopGroup = new QGroupBox("Stop Simulation When...");
    QVBoxLayout* stopLayout = new QVBoxLayout(stopGroup);
    layout->addWidget(stopGroup);

    QWidget* logicRow = new QWidget();
    QHBoxLayout* logicLayout = new QHBoxLayout(logicRow);
    logicLayout->setContentsMargins(0,5,0,5);

    // A. SUBJECT DROPDOWN ("Who?")
    QComboBox* subjectCombo = new QComboBox();
    subjectCombo->addItem("-- Select Subject --", -1);
    for (auto* obj : sceneManager->getObjects()) {
        subjectCombo->addItem(QString::number(obj->getObjectID()), obj->getObjectID()); // TODO: change to object name
    }

    int subjIdx = subjectCombo->findData(sceneManager->stopCondition.subjectID);
    if (subjIdx != -1) subjectCombo->setCurrentIndex(subjIdx);
    else subjectCombo->setCurrentIndex(0);

    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int){
        sceneManager->stopCondition.subjectID = subjectCombo->currentData().toInt();
    });

    // B. PROPERTY DROPDOWN ("What?")
    QComboBox* propCombo = new QComboBox();
    propCombo->addItem("Position Y", 0);
    propCombo->addItem("Velocity Y", 1);
    propCombo->addItem("Distance To", 2);

    propCombo->setCurrentIndex(sceneManager->stopCondition.property);

    // C. OPERATOR DROPDOWN ("How?")
    QComboBox* opCombo = new QComboBox();
    opCombo->addItem("<", 0);
    opCombo->addItem(">", 1);
    opCombo->setCurrentIndex(sceneManager->stopCondition.op);

    // D. TARGET VALUE / OBJECT ("Limit")
    // We use a QStackedWidget to flip between a Number Spinner and an Object Picker
    QStackedWidget* targetStack = new QStackedWidget();

    // Page 0: Scalar Number
    ScalarWidget* valWidget = new ScalarWidget();
    valWidget->setValue(sceneManager->stopCondition.value);
    targetStack->addWidget(valWidget);

    // Page 1: Object Picker (For "Distance To")
    QComboBox* targetObjCombo = new QComboBox();
    for (auto* obj : sceneManager->getObjects()) {
        targetObjCombo->addItem(QString::number(obj->getObjectID()), obj->getObjectID()); // TODO: change to object name
    }
    targetStack->addWidget(targetObjCombo);

    // Logic to switch pages
    auto updateTargetView = [=](int propIdx) {
        if (propIdx == 2) targetStack->setCurrentIndex(1); // Distance -> Show Object Picker
        else targetStack->setCurrentIndex(0);              // Else -> Show Number
    };

    // Initial update
    updateTargetView(propCombo->currentIndex());

    connect(propCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx){
        sceneManager->stopCondition.property = idx;
        updateTargetView(idx);
    });

    // Add widgets to row
    logicLayout->addWidget(subjectCombo);
    logicLayout->addWidget(propCombo);
    logicLayout->addWidget(opCombo);
    logicLayout->addWidget(targetStack);

    stopLayout->addWidget(logicRow);
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

