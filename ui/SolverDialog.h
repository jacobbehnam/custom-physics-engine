#pragma once

#include <QDialog>
#include <QMap>
#include <string>
#include <unordered_map>

class QComboBox;
class ProblemRouter;
class Vector3Widget;
class ScalarWidget;
namespace Physics { class PhysicsBody; }

class SolverDialog : public QDialog {
    Q_OBJECT

public:
    SolverDialog(const ProblemRouter* router, Physics::PhysicsBody* body, QWidget* parent = nullptr);

    std::unordered_map<std::string, double> getCollectedKnowns() const;
    std::string getTargetUnknown() const;
private slots:
    void updateUiState();
    void onSolveClicked();

private:
    void setupUi();
    const ProblemRouter* router;
    Physics::PhysicsBody* targetBody;

    QComboBox* targetCombo;

    QMap<QString, Vector3Widget*> vectorInputs;
    QMap<QString, ScalarWidget*> scalarInputs;
};
