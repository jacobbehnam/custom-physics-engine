#include <graphics/components/Mesh.h>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <graphics/components/Shader.h>

#include "IHandle.h"

class SceneObject;

class RotateHandle : public IHandle{
public:
    RotateHandle(Mesh* m, Shader* sdr, SceneObject* tgt, Axis ax, uint32_t objID);

    void draw() const override;

    Shader * getShader() const override;

    void onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) override;

    void setDragState(glm::vec3 initHitPos) override;

    glm::mat4 getModelMatrix() const override;
    uint32_t getObjectID() const override;
    glm::vec3 getAxisDir() const override {return axisDir(axis);}

    Mesh * getMesh() const override;
private:
    Mesh* mesh;
    Shader* shader;
    SceneObject* target;
    Axis axis;

    float scale = 2.0f;

    glm::vec3 initialHitPoint;
    glm::quat originalQuat;

    uint32_t objectID;
};