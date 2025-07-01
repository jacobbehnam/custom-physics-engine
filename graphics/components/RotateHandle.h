#include <graphics/components/Mesh.h>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <graphics/components/Shader.h>

#include "graphics/interfaces/IHandle.h"

class SceneObject;

class RotateHandle : public IHandle{
public:
    RotateHandle(Mesh* m, Shader* sdr, SceneObject* tgt, Axis ax);
    void draw() const override;

    Shader * getShader() const override;

    void onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) override;

    void setDragState(glm::vec3 initHitPos) override;

    glm::mat4 getModelMatrix() const override;

    Mesh * getMesh() const override;
private:
    Mesh* mesh;
    Shader* shader;
    SceneObject* target;
    Axis axis;

    float scale = 2.0f;

    glm::vec3 initialHitPoint;
    glm::quat originalQuat;
};