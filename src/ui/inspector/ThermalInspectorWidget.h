#pragma once
#include <functional>
#include <vector>
#include "IInspectorSection.h"
#include "InspectorRow.h"

class QFormLayout;
class QTabWidget;
class SceneObject;
class ScalarWidget;
class QString;
namespace Physics { class PhysicsBody; }
struct ThermalProperties;

class ThermalInspectorWidget : public IInspectorSection {
    Q_OBJECT
public:
    explicit ThermalInspectorWidget(QWidget* parent = nullptr);

    void load(SceneObject* object) override;
    void unload() override;
    void refresh() override;

private:
    using ThermalGetter = std::function<double(const ThermalProperties&)>;
    using ThermalSetter = std::function<void(ThermalProperties&, double)>;
    using ScalarInitializer = std::function<void(ScalarWidget*)>;

    SceneObject* selectedObject = nullptr;
    QTabWidget* tabs;
    std::vector<InspectorRow> rows;

    void createUiComponents();
    QFormLayout* createTab(const QString& title);
    void addThermalScalar(QFormLayout* target,
                          const QString& label,
                          const ThermalGetter& get,
                          const ThermalSetter& set,
                          const QString& unit = "",
                          const ScalarInitializer& onInit = nullptr);
    Physics::PhysicsBody* getBody() const;
};
