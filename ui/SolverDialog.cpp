#include "SolverDialog.h"

SolverDialog::SolverDialog(Physics::PhysicsBody *body, QWidget *parent) : QDialog(parent), targetBody(body){
    setWindowTitle("Physics Solver Configuration");
    resize(450, 550);
}

