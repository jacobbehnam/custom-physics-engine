#include "QDialog"

class SolverDialog : public QDialog {
    Q_OBJECT

public:
    explicit SolverDialog(QWidget* parent = nullptr);

    std::unordered_map<std::string, double> getCollectedKnowns() const;
    std::string getTargetUnknown() const;
private slots:
    //void updateUiState();

private:

};
