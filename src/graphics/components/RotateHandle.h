#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

#include "IHandle.h"

class SceneObject;

class RotateHandle : public IHandle{
public:
    RotateHandle(SceneObject* tgt, Axis ax);

    void onDrag(const Math::Ray& ray) override;
    void setDragState(glm::vec3 initHitPos) override;
    glm::mat4 getModelMatrix() const override;
    glm::vec3 getAxisDir() const override { return axisDir(axis); }

private:
    SceneObject* target;
    Axis axis;

    constexpr static float scale = 2.0f;

    glm::vec3 initialHitPoint;
    glm::quat originalQuat;
};