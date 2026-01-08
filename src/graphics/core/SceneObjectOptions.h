#pragma once
#include <glm/glm.hpp>
#include <variant>
#include <functional>

#include "physics/bounding/BoxCollider.h"

class ICollider;

struct ObjectOptions {
    glm::vec3 position{0.0f};
    glm::vec3 scale{1.0f};
    glm::vec3 rotation{0.0f};
};

struct PointMassOptions {
    ObjectOptions base;
    bool isStatic = false;
    float mass = 1.0f;

    PointMassOptions() = default;
};

struct RigidBodyOptions {
    ObjectOptions base;
    std::function<std::unique_ptr<Physics::Bounding::ICollider>(const ObjectOptions&)> createCollider;
    bool isStatic = false;
    float mass = 1.0f;

    // Helpers for making colliders
    static RigidBodyOptions Box(ObjectOptions base, bool isStatic = false, float mass = 1.0f) {
        RigidBodyOptions o;
        o.base = base;
        o.mass = mass;
        o.isStatic = isStatic;
        o.createCollider = [](auto const& b) -> std::unique_ptr<Physics::Bounding::ICollider>{
            return std::make_unique<Physics::Bounding::BoxCollider>(
                b.position,
                b.scale / 2.0f,
                b.rotation
            );
        };
        return o;
    }
};

using CreationOptions = std::variant<ObjectOptions, PointMassOptions, RigidBodyOptions>;
