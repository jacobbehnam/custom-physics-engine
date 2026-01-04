#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <glm/glm.hpp>

class Vector3Widget : public QWidget{
    Q_OBJECT

public:
    explicit Vector3Widget(const QString& suffix = "", QWidget* parent = nullptr);

    glm::vec3 getValue() const;
    void setValue(const glm::vec3& v);

signals:
    void valueChanged(const glm::vec3& newValue);
private:
    QDoubleSpinBox *xSpin, *ySpin, *zSpin;
};
