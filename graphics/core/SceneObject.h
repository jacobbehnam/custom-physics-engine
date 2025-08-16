#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <graphics/components/Mesh.h>
#include <graphics/interfaces/IDrawable.h>
#include <graphics/components/Shader.h>

#include "SceneManager.h"
#include "graphics/interfaces/IPickable.h"
#include "physics/PhysicsSystem.h"
#include "graphics/core/SceneObjectOptions.h"

class Scene;
class SceneManager;
namespace Physics {
    class RigidBody;
}

// TODO: put setters and getter definitions in the header files
class SceneObject : public QObject, public IDrawable, public IPickable{
    Q_OBJECT
public:
    SceneObject(SceneManager* sceneManager, const std::string &meshName, Shader *sdr, const CreationOptions &options, QObject* parent = nullptr);
    ~SceneObject();

    void draw() const override;

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
    bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) override;

    void handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) override;
    void setHovered(bool hovered) override;
    bool getHovered() override;
    Mesh* getMesh() const override { return mesh; }
    std::string getMeshName() const { return meshName; }
    uint32_t getObjectID() const override;
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
    std::string meshName;
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

    glm::mat4 getModelMatrix() const;

    bool intersectsAABB(const glm::vec3& orig, const glm::vec3& dir, float& outT) const;
    bool intersectsMesh(const glm::vec3& orig, const glm::vec3& dir, float& outT) const;
};