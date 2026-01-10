#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility>
#include <graphics/components/Mesh.h>
#include "IDrawable.h"
#include <graphics/components/Shader.h>

#include "SceneManager.h"
#include "IPickable.h"
#include "physics/PhysicsSystem.h"
#include "graphics/core/SceneObjectOptions.h"

class Scene;
class SceneManager;
namespace Physics {
    class RigidBody;
}

// TODO: put setters and getter definitions in the header files
class SceneObject : public QObject, public IInstancedDrawable, public IPickable{
    Q_OBJECT
public:
    SceneObject(SceneManager* sceneManager, const std::string &meshName, Shader *sdr, const CreationOptions &options, QObject* parent = nullptr);
    ~SceneObject();

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setRotationQuat(const glm::quat& q);
    void setScale(const glm::vec3& scl);
    glm::vec3 getPosition() const;
    glm::vec3 getRotation() const;
    glm::quat getRotationQuat() const;
    glm::vec3 getScale() const;

    Shader* getShader() const override;
    Physics::PhysicsBody* getPhysicsBody() const { return physicsBody.get(); }
    std::optional<float> intersectsRay(const Math::Ray& ray) const override;

    void handleClick(const Math::Ray& ray, float distance) override;
    void setHovered(bool hovered) override;
    bool getHovered() const override;
    Mesh* getMesh() const override { return mesh; }
    uint32_t getObjectID() const override;
    const std::string& getName() const { return objectName; }
    void setName(std::string name) { objectName = std::move(name); }
    CreationOptions getCreationOptions() const { return creationOptions; }

    using PosMap = std::unordered_map<Physics::PhysicsBody*, glm::vec3>;

    static void setPhysicsPosMap(const PosMap& m) {
        std::lock_guard<std::mutex> lk(posMapMutex);
        posMap = m;
    }

private:
    inline static std::mutex posMapMutex;
    inline static PosMap posMap{};

    QObject* parent;
    Mesh* mesh;
    Shader* shader;
    Scene* ownerScene;
    SceneManager* sceneManager;
    std::unique_ptr<Physics::PhysicsBody> physicsBody = nullptr;

    CreationOptions creationOptions;
    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::quat orientation {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale {1.0f};

    bool isHovered = false;

    uint32_t objectID;
    std::string objectName;

    glm::mat4 getModelMatrix() const;

    std::optional<float> intersectsAABB(const Math::Ray& ray) const;
    std::optional<float> intersectsMesh(const Math::Ray& ray) const;
};