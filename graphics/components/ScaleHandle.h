#include "graphics/interfaces/IHandle.h"
#include <graphics/core/SceneObject.h>

class ScaleHandle : public IHandle{
public:
    ScaleHandle(Mesh* m, Shader* sdr, SceneObject* tgt, Axis ax);
    void draw() const override;

    Shader * getShader() const override;

    void onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) override;

    void setDragState(glm::vec3 initHitPos) override;

    glm::mat4 getModelMatrix() const override;
    Mesh* getMesh() const override;
private:
    Mesh* mesh;
    Shader* shader;
    SceneObject* target;
    Axis axis;

    float thickness = 0.5f;
    float length = 1.0f;

    glm::vec3 initialHitPoint;
    glm::vec3 originalScale;
};
