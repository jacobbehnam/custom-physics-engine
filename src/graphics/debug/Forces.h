#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "graphics/components/Shader.h"
#include "graphics/core/InstanceData.h"
#include "graphics/core/SceneObject.h"
#include "graphics/core/IDrawable.h"

class SceneManager;

class Forces : public ICustomDrawable {
public:
    Forces(SceneManager* sceneManager);
    ~Forces() override = default;

    Shader* getShader() const override { return basicShader; }
    Mesh* getMesh() const override { return nullptr; }
    uint32_t getObjectID() const override { return 0; }

    void draw() const override;

    void setEnabled(bool value) { enabled = value; }

private:
    struct ArrowCpu {
        uint32_t objectID = 0;
        glm::vec3 net{0.0f};
        float netMag = 0.0f;
        glm::vec3 startPos{0.0f};
    };

    SceneManager* sceneManager;
    Shader* basicShader = nullptr;
    bool enabled = true;

    /** Reused each draw to avoid per-frame heap allocations when forces overlay is on. */
    mutable std::vector<ArrowCpu> m_arrowScratch;
    mutable std::vector<Rendering::InstanceData> m_instanceScratch;
};