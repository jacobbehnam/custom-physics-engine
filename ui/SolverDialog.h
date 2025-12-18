#pragma once
#include "QDialog"
#include "physics/PhysicsBody.h"

class SolverDialog : public QDialog {
    Q_OBJECT

public:
    SolverDialog(Physics::PhysicsBody* body, QWidget* parent = nullptr);

    std::unordered_map<std::string, double> getCollectedKnowns() const;
    std::string getTargetUnknown() const;
private slots:
    //void updateUiState();

private:
    Physics::PhysicsBody* targetBody;
};
