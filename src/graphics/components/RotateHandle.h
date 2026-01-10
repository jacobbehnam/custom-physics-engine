#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

#include "IHandle.h"

class SceneObject;

class RotateHandle : public IHandle{
public:
    RotateHandle(SceneObject* tgt, Axis ax, uint32_t objID);

    void onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) override;
    void setDragState(glm::vec3 initHitPos) override;
    glm::mat4 getModelMatrix() const override;
    glm::vec3 getAxisDir() const override { return axisDir(axis); }
    uint32_t getObjectID() const override { return objectID; }
private:
    SceneObject* target;
    Axis axis;

    constexpr static float scale = 2.0f;

    glm::vec3 initialHitPoint;
    glm::quat originalQuat;

    uint32_t objectID;
};