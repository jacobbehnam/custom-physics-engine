#include <graphics/interfaces/IDrawable.h>
#include <graphics/components/Mesh.h>
#include <glm/glm.hpp>
#include <graphics/components/Shader.h>

class SceneObject;

enum class Axis {
    X, Y, Z
};

class RotateHandle : public IDrawable{
    RotateHandle(Mesh* m, Shader* sdr, SceneObject* tgt, Axis ax);

};