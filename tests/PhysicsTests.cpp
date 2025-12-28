#include <gtest/gtest.h>
#include "physics/PhysicsSystem.h"
#include "physics/PointMass.h"

// Helper Macro for concise GLM comparisons
#define EXPECT_VEC3_EQ(vec, x_val, y_val, z_val) \
    EXPECT_FLOAT_EQ((vec).x, (x_val)); \
    EXPECT_FLOAT_EQ((vec).y, (y_val)); \
    EXPECT_FLOAT_EQ((vec).z, (z_val))

// Global Test Constants
static constexpr float MACHINE_EPSILON = std::numeric_limits<float>::epsilon();
static constexpr float OPS_SAFETY_FACTOR = 8.0f; // The conservative constant 'C' from TESTING.md
static constexpr float BASE_FLOAT_TOLERANCE = MACHINE_EPSILON * OPS_SAFETY_FACTOR;

TEST(PointMass, Constructor_Default) {
    auto pm = Physics::PointMass(0, 1.0f);

    EXPECT_EQ(pm.getID(), 0);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 1.f);
    EXPECT_VEC3_EQ(pm.getPosition(BodyLock::LOCK), 0.0f, 0.0f, 0.0f);
    EXPECT_FALSE(pm.getIsStatic(BodyLock::LOCK));
}

TEST(PointMass, Constructor_Explicit) {
    auto pm = Physics::PointMass(0, 1.5f, glm::vec3(1.0f, -2.5f, 0.0f), false);

    EXPECT_EQ(pm.getID(), 0);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 1.5f);
    EXPECT_VEC3_EQ(pm.getPosition(BodyLock::LOCK), 1.0f, -2.5f, 0.0f);
    EXPECT_FALSE(pm.getIsStatic(BodyLock::LOCK));
}

TEST(PointMass, Simple_Set) {
    auto pm = Physics::PointMass(0, 1.5f, glm::vec3(1.0f, -2.5f, 0.0f), false);

    EXPECT_EQ(pm.getID(), 0);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 1.5f);
    EXPECT_VEC3_EQ(pm.getPosition(BodyLock::LOCK), 1.0f, -2.5f, 0.0f);
    EXPECT_FALSE(pm.getIsStatic(BodyLock::LOCK));

    pm.setVelocity(glm::vec3(0.0f), BodyLock::LOCK);
    EXPECT_VEC3_EQ(pm.getVelocity(BodyLock::LOCK), 0.0f, 0.0f, 0.0f);

    pm.setMass(2.3f, BodyLock::LOCK);
    pm.setVelocity(glm::vec3(1.0f, 1.0f, -2.4f), BodyLock::LOCK);
    pm.setPosition(glm::vec3(1.0f, 0.0f, 0.0f), BodyLock::LOCK);

    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 2.3f);
    EXPECT_VEC3_EQ(pm.getPosition(BodyLock::LOCK), 1.0f, 0.0f, 0.0f);
    EXPECT_VEC3_EQ(pm.getVelocity(BodyLock::LOCK), 1.0f, 1.0f, -2.4f);
}

TEST(PointMass, Force_Simple_Add) {
    auto pm = Physics::PointMass(0, 1.0f);
    pm.setForce("Fa", glm::vec3(0.0f, 1.0f, 0.0f), BodyLock::LOCK);
    EXPECT_VEC3_EQ(pm.getForce("Fa", BodyLock::LOCK), 0.0f, 1.0f, 0.0f);
}

TEST(PointMass, Force_Complex_Add) {
    auto pm = Physics::PointMass(0, 1.0f);
    pm.setForce("Fa1", glm::vec3(0.0f, 1.0f, 0.0f), BodyLock::LOCK);
    pm.setForce("Fa2", glm::vec3(1.0f, 0.0f, -1.0f), BodyLock::LOCK);
    pm.setForce("Fa3", glm::vec3(0.0f, 2.5f, -0.25f), BodyLock::LOCK);
    EXPECT_VEC3_EQ(pm.getForce("Fa1", BodyLock::LOCK), 0.0f, 1.0f, 0.0f);
    EXPECT_VEC3_EQ(pm.getForce("Fa2", BodyLock::LOCK), 1.0f, 0.0f, -1.0f);
    EXPECT_VEC3_EQ(pm.getForce("Fa3", BodyLock::LOCK), 0.0f, 2.5f, -0.25f);

    EXPECT_VEC3_EQ(pm.getNetForce(BodyLock::LOCK), 1.0f, 3.5, -1.25f);

    pm.setForce("Fa4", glm::vec3(0.0f), BodyLock::LOCK);
    EXPECT_VEC3_EQ(pm.getForce("Fa4", BodyLock::LOCK), 0.0f, 0.0f, 0.0f);
    EXPECT_VEC3_EQ(pm.getNetForce(BodyLock::LOCK), 1.0f, 3.5, -1.25f);
}

TEST(PointMass, Force_Update_Existing) {
    auto pm = Physics::PointMass(0, 1.0f);

    pm.setForce("Thrust", glm::vec3(0.0f, 10.0f, 0.0f), BodyLock::LOCK);
    EXPECT_VEC3_EQ(pm.getNetForce(BodyLock::LOCK), 0.0f, 10.0f, 0.0f);

    pm.setForce("Thrust", glm::vec3(0.0f, 50.0f, 0.0f), BodyLock::LOCK);

    EXPECT_VEC3_EQ(pm.getForce("Thrust", BodyLock::LOCK), 0.0f, 50.0f, 0.0f);
    EXPECT_VEC3_EQ(pm.getNetForce(BodyLock::LOCK), 0.0f, 50.0f, 0.0f);
}

TEST(PointMass, Force_Get_NonExistent) {
    auto pm = Physics::PointMass(0, 1.0f);

    EXPECT_VEC3_EQ(pm.getForce("GhostForce", BodyLock::LOCK), 0.0f, 0.0f, 0.0f);
    EXPECT_VEC3_EQ(pm.getNetForce(BodyLock::LOCK), 0.0f, 0.0f, 0.0f);
}

TEST(PointMass, Invalid_Mass) {
    auto pm = Physics::PointMass(0, 0.0f);
    // Mass defaults to 1.0, should ignore invalid mass values
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 1.0f);
    pm.setMass(-1.25, BodyLock::LOCK);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 1.0f);
    pm.setMass(2.0f, BodyLock::LOCK);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 2.0f);
    pm.setMass(0.0f, BodyLock::LOCK);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 2.0f);
}

TEST(PointMass, Concurrency_ManualLock) {
    auto pm = Physics::PointMass(0, 1.0f);

    {
        auto externalLock = pm.lockState();

        pm.setPosition(glm::vec3(5.0f), BodyLock::NOLOCK);
        pm.setVelocity(glm::vec3(10.0f), BodyLock::NOLOCK);

        EXPECT_VEC3_EQ(pm.getPosition(BodyLock::NOLOCK), 5.0f, 5.0f, 5.0f);
    }
}

TEST(PointMass, State_Unknown_Flags) {
    auto pm = Physics::PointMass(0, 1.0f);

    EXPECT_FALSE(pm.isUnknown("v0", BodyLock::LOCK));
    EXPECT_FALSE(pm.isUnknown("x0", BodyLock::LOCK));

    pm.setUnknown("v0", true, BodyLock::LOCK);
    EXPECT_TRUE(pm.isUnknown("v0", BodyLock::LOCK));
    EXPECT_FALSE(pm.isUnknown("x0", BodyLock::LOCK));

    pm.setUnknown("x0", true, BodyLock::LOCK);
    EXPECT_TRUE(pm.isUnknown("v0", BodyLock::LOCK));
    EXPECT_TRUE(pm.isUnknown("x0", BodyLock::LOCK));

    pm.setUnknown("v0", false, BodyLock::LOCK);
    EXPECT_FALSE(pm.isUnknown("v0", BodyLock::LOCK));
    EXPECT_TRUE(pm.isUnknown("x0", BodyLock::LOCK));
}

TEST(PointMass, State_Static_Toggle) {
    auto pm = Physics::PointMass(0, 1.0f);
    pm.setVelocity(glm::vec3(10.0f), BodyLock::LOCK);

    pm.setIsStatic(true, BodyLock::LOCK);
    EXPECT_TRUE(pm.getIsStatic(BodyLock::LOCK));

    EXPECT_VEC3_EQ(pm.getVelocity(BodyLock::LOCK), 10.0f, 10.0f, 10.0f);

    pm.setIsStatic(false, BodyLock::LOCK);
    EXPECT_FALSE(pm.getIsStatic(BodyLock::LOCK));
}

TEST(PhysicsSystem, Management_AddGetRemove) {
    Physics::PhysicsSystem system;
    Physics::PointMass pm(10, 1.0f);
    EXPECT_EQ(system.getBodyById(10), nullptr);

    system.addBody(&pm);
    EXPECT_EQ(system.getBodyById(10), &pm);

    system.removeBody(&pm);
    EXPECT_EQ(system.getBodyById(10), nullptr);
}

TEST(PhysicsSystem, Parameters_GlobalSettings) {
    Physics::PhysicsSystem system;

    EXPECT_FLOAT_EQ(system.getGlobalAcceleration().y, -9.81f);
    EXPECT_FLOAT_EQ(system.getSimSpeed(), 1.0f);

    system.setGlobalAcceleration(glm::vec3(0.0f));
    EXPECT_VEC3_EQ(system.getGlobalAcceleration(), 0.0f, 0.0f, 0.0f);

    system.setSimSpeed(0.5f);
    EXPECT_FLOAT_EQ(system.getSimSpeed(), 0.5f);
}

// Simulation tests
TEST(Integration, Freefall_ConstantAcceleration) {
    float gravity = -9.81f;
    float time = 1.0f;
    float dt = 0.01f;
    float startY = 100.0f;

    Physics::PhysicsSystem system(glm::vec3(0, gravity, 0));
    Physics::PointMass pm(0, 1.0f);
    pm.setPosition(glm::vec3(0, startY, 0), BodyLock::LOCK);
    system.addBody(&pm);

    int steps = static_cast<int>(time / dt);
    for(int i = 0; i < steps; i++) system.step(dt);
    EXPECT_NEAR(system.simTime, time, dt/2); // Make sure we are in the right frame

    // Computed analytically
    constexpr float exactY = 95.095;
    float maxAbsPos = startY;

    float errorBound = steps * BASE_FLOAT_TOLERANCE * maxAbsPos;
    EXPECT_NEAR(pm.getPosition(BodyLock::LOCK).y, exactY, errorBound);
}