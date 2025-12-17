#include "QDialog"

class SolverDialog : public QDialog {
    Q_OBJECT

public:
    explicit SolverDialog(QWidget* parent = nullptr);

    double getSolverValue() const;
private:

};
