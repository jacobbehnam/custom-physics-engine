#include "SolverDialog.h"
#include "ui/Vector3Widget.h"
#include "ui/ScalarWidget.h"
#include "physics/PhysicsBody.h"
#include "physics/solver/ProblemRouter.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <qpushbutton.h>

SolverDialog::SolverDialog(const ProblemRouter* problemRouter, Physics::PhysicsBody *body, QWidget *parent) : QDialog(parent), targetBody(body), router(problemRouter){
    setWindowTitle("Physics Solver Configuration");
    resize(450, 550);
    setupUi();
    updateUiState();
}

void SolverDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* objGroup = new QGroupBox("1. Problem Definition");
    QFormLayout* objLayout = new QFormLayout(objGroup);

    QLabel* nameLabel = new QLabel("placeholder");
    objLayout->addRow("Active Body:", nameLabel);

    targetCombo = new QComboBox();
    targetCombo->addItem("Initial Velocity (v0)", "v0");
    targetCombo->addItem("Target Position (rT)", "rT");
    targetCombo->addItem("Flight Duration (T)", "T");

    objLayout->addRow("Solve For:", targetCombo);
    mainLayout->addWidget(objGroup);
    connect(targetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SolverDialog::updateUiState);

    QGroupBox* paramGroup = new QGroupBox("2. Known Parameters");
    QFormLayout* formLayout = new QFormLayout(paramGroup);

    auto addVector = [&](const QString& label, const QString& key) {
        Vector3Widget* widget = new Vector3Widget();
        vectorInputs[key] = widget;
        formLayout->addRow(label, widget);
    };

    auto addScalar = [&](const QString& label, const QString& key, QString suffix) {
        ScalarWidget* widget = new ScalarWidget(suffix);

        if (key == "T") widget->setRange(0.001, 10000.0);

        scalarInputs[key] = widget;
        formLayout->addRow(label, widget);
    };

    formLayout->addRow(new QLabel("<b>Initial State</b>"));
    addVector("Position (r0):", "r0");
    addVector("Velocity (v0):", "v0");

    formLayout->addRow(new QLabel("<b>Target State</b>"));
    addVector("Position (rT):", "rT");
    addVector("Velocity (vT):", "vT");

    formLayout->addRow(new QLabel("<b>Constraints</b>"));
    addScalar("Duration (T):", "T", " s");

    mainLayout->addWidget(paramGroup);
    mainLayout->addStretch();

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Solve");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SolverDialog::onSolveClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

std::string SolverDialog::getTargetUnknown() const {
    return targetCombo->currentData().toString().toStdString();
}

std::unordered_map<std::string, double> SolverDialog::getCollectedKnowns() const {
    std::unordered_map<std::string, double> knowns;

    // Flatten Vector3Widgets into x/y/z keys
    for (auto it = vectorInputs.begin(); it != vectorInputs.end(); ++it) {
        if (it.value()->isEnabled()) {
            glm::vec3 val = it.value()->getValue();
            std::string k = it.key().toStdString();
            knowns[k + "_x"] = val.x;
            knowns[k + "_y"] = val.y;
            knowns[k + "_z"] = val.z;
        }
    }

    // Collect Scalars
    for (auto it = scalarInputs.begin(); it != scalarInputs.end(); ++it) {
        if (it.value()->isEnabled()) {
            knowns[it.key().toStdString()] = it.value()->getValue();
        }
    }

    return knowns;
}

void SolverDialog::updateUiState() {
    for(auto* w : vectorInputs) w->setEnabled(false);
    for(auto* w : scalarInputs) w->setEnabled(false);

    std::string unknown = getTargetUnknown();
    auto requirements = router->getRequiredKeys(unknown);
    if (requirements.empty()) return;

    // Enable needed fields
    for (const std::string& key : requirements[0]) {
        QString qKey = QString::fromStdString(key);

        if (scalarInputs.contains(qKey)) {
            scalarInputs[qKey]->setEnabled(true);
        }
        else if (qKey.endsWith("_x")) {
            QString base = qKey.left(qKey.length() - 2);
            if (vectorInputs.contains(base)) {
                vectorInputs[base]->setEnabled(true);
            }
        }
    }
}

void SolverDialog::onSolveClicked() {
    accept();
}