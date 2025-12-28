#include <gtest/gtest.h>
#include "physics/PhysicsSystem.h"
#include "physics/PointMass.h"

// Helper Macros for concise GLM comparisons
#define EXPECT_VEC3_EXACT(actual, expected) \
    EXPECT_FLOAT_EQ((actual).x, (expected).x); \
    EXPECT_FLOAT_EQ((actual).y, (expected).y); \
    EXPECT_FLOAT_EQ((actual).z, (expected).z)

#define EXPECT_VEC3_NEAR(actual, expected, tol) \
    EXPECT_NEAR((actual).x, (expected).x, (tol)); \
    EXPECT_NEAR((actual).y, (expected).y, (tol)); \
    EXPECT_NEAR((actual).z, (expected).z, (tol))

// Global Test Constants
static constexpr float MACHINE_EPSILON = std::numeric_limits<float>::epsilon();
static constexpr float OPS_SAFETY_FACTOR = 8.0f; // The conservative constant 'C' from TESTING.md
static constexpr float BASE_FLOAT_TOLERANCE = MACHINE_EPSILON * OPS_SAFETY_FACTOR;

TEST(PointMass, Constructor_Default) {
    auto pm = Physics::PointMass(0, 1.0f);

    EXPECT_EQ(pm.getID(), 0);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 1.f);
    EXPECT_VEC3_EXACT(pm.getPosition(BodyLock::LOCK), glm::vec3(0.0f));
    EXPECT_FALSE(pm.getIsStatic(BodyLock::LOCK));
}

TEST(PointMass, Constructor_Explicit) {
    auto pm = Physics::PointMass(0, 1.5f, glm::vec3(1.0f, -2.5f, 0.0f), false);

    EXPECT_EQ(pm.getID(), 0);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 1.5f);
    EXPECT_VEC3_EXACT(pm.getPosition(BodyLock::LOCK), glm::vec3(1.0f, -2.5f, 0.0f));
    EXPECT_FALSE(pm.getIsStatic(BodyLock::LOCK));
}

TEST(PointMass, Simple_Set) {
    auto pm = Physics::PointMass(0, 1.5f, glm::vec3(1.0f, -2.5f, 0.0f), false);

    EXPECT_EQ(pm.getID(), 0);
    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 1.5f);
    EXPECT_VEC3_EXACT(pm.getPosition(BodyLock::LOCK), glm::vec3(1.0f, -2.5f, 0.0f));
    EXPECT_FALSE(pm.getIsStatic(BodyLock::LOCK));

    pm.setVelocity(glm::vec3(0.0f), BodyLock::LOCK);
    EXPECT_VEC3_EXACT(pm.getVelocity(BodyLock::LOCK), glm::vec3(0.0f, 0.0f, 0.0f));

    pm.setMass(2.3f, BodyLock::LOCK);
    pm.setVelocity(glm::vec3(1.0f, 1.0f, -2.4f), BodyLock::LOCK);
    pm.setPosition(glm::vec3(1.0f, 0.0f, 0.0f), BodyLock::LOCK);

    EXPECT_FLOAT_EQ(pm.getMass(BodyLock::LOCK), 2.3f);
    EXPECT_VEC3_EXACT(pm.getPosition(BodyLock::LOCK), glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_VEC3_EXACT(pm.getVelocity(BodyLock::LOCK), glm::vec3(1.0f, 1.0f, -2.4f));
}

TEST(PointMass, Force_Simple_Add) {
    auto pm = Physics::PointMass(0, 1.0f);
    pm.setForce("Fa", glm::vec3(0.0f, 1.0f, 0.0f), BodyLock::LOCK);
    EXPECT_VEC3_EXACT(pm.getForce("Fa", BodyLock::LOCK), glm::vec3(0.0f, 1.0f, 0.0f));
}

TEST(PointMass, Force_Complex_Add) {
    auto pm = Physics::PointMass(0, 1.0f);
    pm.setForce("Fa1", glm::vec3(0.0f, 1.0f, 0.0f), BodyLock::LOCK);
    pm.setForce("Fa2", glm::vec3(1.0f, 0.0f, -1.0f), BodyLock::LOCK);
    pm.setForce("Fa3", glm::vec3(0.0f, 2.5f, -0.25f), BodyLock::LOCK);
    EXPECT_VEC3_EXACT(pm.getForce("Fa1", BodyLock::LOCK), glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_VEC3_EXACT(pm.getForce("Fa2", BodyLock::LOCK), glm::vec3(1.0f, 0.0f, -1.0f));
    EXPECT_VEC3_EXACT(pm.getForce("Fa3", BodyLock::LOCK), glm::vec3(0.0f, 2.5f, -0.25f));

    EXPECT_VEC3_EXACT(pm.getNetForce(BodyLock::LOCK), glm::vec3(1.0f, 3.5, -1.25f));

    pm.setForce("Fa4", glm::vec3(0.0f), BodyLock::LOCK);
    EXPECT_VEC3_EXACT(pm.getForce("Fa4", BodyLock::LOCK), glm::vec3(0.0f, 0.0f, 0.0f));
    EXPECT_VEC3_EXACT(pm.getNetForce(BodyLock::LOCK), glm::vec3(1.0f, 3.5, -1.25f));
}

TEST(PointMass, Force_Update_Existing) {
    auto pm = Physics::PointMass(0, 1.0f);

    pm.setForce("Thrust", glm::vec3(0.0f, 10.0f, 0.0f), BodyLock::LOCK);
    EXPECT_VEC3_EXACT(pm.getNetForce(BodyLock::LOCK), glm::vec3(0.0f, 10.0f, 0.0f));

    pm.setForce("Thrust", glm::vec3(0.0f, 50.0f, 0.0f), BodyLock::LOCK);

    EXPECT_VEC3_EXACT(pm.getForce("Thrust", BodyLock::LOCK), glm::vec3(0.0f, 50.0f, 0.0f));
    EXPECT_VEC3_EXACT(pm.getNetForce(BodyLock::LOCK), glm::vec3(0.0f, 50.0f, 0.0f));
}

TEST(PointMass, Force_Get_NonExistent) {
    auto pm = Physics::PointMass(0, 1.0f);

    EXPECT_VEC3_EXACT(pm.getForce("GhostForce", BodyLock::LOCK), glm::vec3(0.0f, 0.0f, 0.0f));
    EXPECT_VEC3_EXACT(pm.getNetForce(BodyLock::LOCK), glm::vec3(0.0f, 0.0f, 0.0f));
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

        EXPECT_VEC3_EXACT(pm.getPosition(BodyLock::NOLOCK), glm::vec3(5.0f, 5.0f, 5.0f));
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

    EXPECT_VEC3_EXACT(pm.getVelocity(BodyLock::LOCK), glm::vec3(10.0f, 10.0f, 10.0f));

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
    EXPECT_VEC3_EXACT(system.getGlobalAcceleration(), glm::vec3(0.0f, 0.0f, 0.0f));

    system.setSimSpeed(0.5f);
    EXPECT_FLOAT_EQ(system.getSimSpeed(), 0.5f);
}

// Simulation tests
TEST(Integration, Zero_Velocity_Zero_Acceleration) {
    constexpr float time = 50.0f;
    constexpr float dt = 0.01f;

    Physics::PhysicsSystem system(glm::vec3(0.0f));
    Physics::PointMass pm(0, 1.0f);
    system.addBody(&pm);
    pm.setPosition(glm::vec3(0.0f), BodyLock::LOCK);

    constexpr auto steps = static_cast<size_t>(time / dt);
    for(size_t i = 0; i < steps; i++) system.step(dt);
    EXPECT_NEAR(system.simTime, time, dt/2);

    constexpr auto expected = glm::vec3(0.0f);
    constexpr double maxAbsPos = 0.0;

    constexpr double errorBound = steps * BASE_FLOAT_TOLERANCE * maxAbsPos;
    EXPECT_VEC3_NEAR(pm.getPosition(BodyLock::LOCK), expected, errorBound);
}

TEST(Integration, Constant_Velocity_Zero_Acceleration) {
    constexpr float time = 10.0f;
    constexpr float dt = 0.01f;

    Physics::PhysicsSystem system(glm::vec3(0.0f));
    Physics::PointMass pm(0, 1.0f);
    system.addBody(&pm);
    pm.setPosition(glm::vec3(0.0f), BodyLock::LOCK);
    pm.setVelocity(glm::vec3(1.0f), BodyLock::LOCK);

    constexpr auto steps = static_cast<size_t>(time / dt);
    for(size_t i = 0; i < steps; i++) system.step(dt);
    EXPECT_NEAR(system.simTime, time, dt/2);

    constexpr auto expected = glm::vec3(10.0f);
    double maxAbsPos = glm::distance(glm::vec3(0.0f), expected);

    double errorBound = steps * BASE_FLOAT_TOLERANCE * maxAbsPos;
    EXPECT_VEC3_NEAR(pm.getPosition(BodyLock::LOCK), expected, errorBound);
}

TEST(Integration, Freefall_Zero_Initial_Velocity) {
    constexpr float gravity = -9.81f;
    constexpr float time = 1.0f;
    constexpr float dt = 0.01f;
    constexpr float startY = 100.0f;

    Physics::PhysicsSystem system(glm::vec3(0, gravity, 0));
    Physics::PointMass pm(0, 1.0f);
    system.addBody(&pm);
    pm.setPosition(glm::vec3(0, startY, 0), BodyLock::LOCK);

    constexpr auto steps = static_cast<size_t>(time / dt);
    for(size_t i = 0; i < steps; i++) system.step(dt);
    EXPECT_NEAR(system.simTime, time, dt/2); // Make sure we are in the right frame

    // Computed analytically
    constexpr auto expected = glm::vec3(0.0f, 95.095f, 0.0f);
    constexpr double maxAbsPos = startY;

    constexpr double errorBound = steps * BASE_FLOAT_TOLERANCE * maxAbsPos;
    EXPECT_VEC3_NEAR(pm.getPosition(BodyLock::LOCK), expected, errorBound);
}

TEST(Integration, Freefall_With_Initial_Velocity) {
    constexpr float gravity = -9.81f;
    constexpr float time = 10.0f;
    constexpr float dt = 0.01f;

    Physics::PhysicsSystem system(glm::vec3(0, gravity, 0));
    Physics::PointMass pm(0, 1.0f);
    system.addBody(&pm);
    pm.setPosition(glm::vec3(0.0f), BodyLock::LOCK);
    pm.setVelocity(glm::vec3(0.0f, 10.0f, 0.0f), BodyLock::LOCK);

    constexpr auto steps = static_cast<size_t>(time / dt);
    for(size_t i = 0; i < steps; i++) system.step(dt);
    EXPECT_NEAR(system.simTime, time, dt/2); // Make sure we are in the right frame

    // Computed analytically
    constexpr auto expected = glm::vec3(0.0f, -390.5f, 0.0f);
    constexpr double maxAbsPos = 390.5;

    constexpr double errorBound = steps * BASE_FLOAT_TOLERANCE * maxAbsPos;
    EXPECT_VEC3_NEAR(pm.getPosition(BodyLock::LOCK), expected, errorBound);
}

TEST(Integration, Multi_Axis_Forces) {
    // Forces: Gravity on Y axis, Thrust on X and Y axis
    constexpr float gravity = -9.81f;
    constexpr float time = 2.5f;
    constexpr float dt = 0.01f;

    Physics::PhysicsSystem system(glm::vec3(0, gravity, 0));
    Physics::PointMass pm(0, 1.0f);
    system.addBody(&pm);
    pm.setPosition(glm::vec3(0.0f), BodyLock::LOCK);
    pm.setVelocity(glm::vec3(5.0f, 10.0f, 0.0f), BodyLock::LOCK);
    pm.setForce("Thrust", glm::vec3(5.0f, 5.0f, 0.0f), BodyLock::LOCK);

    constexpr auto steps = static_cast<size_t>(time / dt);
    for(size_t i = 0; i < steps; i++) system.step(dt);
    EXPECT_NEAR(system.simTime, time, dt/2); // Make sure we are in the right frame

    // Computed analytically
    constexpr auto expected = glm::vec3(28.125f, 9.96875f, 0.0f);
    double maxAbsPos = glm::distance(glm::vec3(0.0f), expected);

    double errorBound = steps * BASE_FLOAT_TOLERANCE * maxAbsPos;
    EXPECT_VEC3_NEAR(pm.getPosition(BodyLock::LOCK), expected, errorBound);
}

TEST(Integration, Mass_Scaling_Constant_Force) {
    constexpr float time = 5.0f;
    constexpr float dt = 0.01f;
    constexpr float massLight = 1.0f;
    constexpr float massHeavy = 4.0f;

    Physics::PhysicsSystem system(glm::vec3(0.0));
    Physics::PointMass light(0, massLight);
    Physics::PointMass heavy(1, massHeavy);
    light.setPosition(glm::vec3(-1.0f, 0.0f, 0.0f), BodyLock::LOCK);
    heavy.setPosition(glm::vec3(1.0f, 0.0f, 0.0f), BodyLock::LOCK);
    light.setVelocity(glm::vec3(0.0f), BodyLock::LOCK);
    heavy.setVelocity(glm::vec3(0.0f), BodyLock::LOCK);

    system.addBody(&light);
    system.addBody(&heavy);
    light.setForce("DownForce", glm::vec3(0.0, -1.0f, 0.0f), BodyLock::LOCK);
    heavy.setForce("DownForce", glm::vec3(0.0f, -1.0f, 0.0f), BodyLock::LOCK);

    constexpr auto steps = static_cast<size_t>(time / dt);
    for(size_t i = 0; i < steps; i++) system.step(dt);
    EXPECT_NEAR(system.simTime, time, dt/2); // Make sure we are in the right frame

    constexpr auto expectedLight = glm::vec3(-1.0f, -12.5f, 0.0f);
    constexpr auto expectedHeavy = glm::vec3(1.0f, -3.125f, 0.0f);
    double maxAbsPosLight = glm::distance(glm::vec3(0.0f), expectedLight);
    double maxAbsPosHeavy = glm::distance(glm::vec3(0.0f), expectedHeavy);

    double errorLight = steps * BASE_FLOAT_TOLERANCE * maxAbsPosLight;
    double errorHeavy = steps * BASE_FLOAT_TOLERANCE * maxAbsPosHeavy;
    EXPECT_VEC3_NEAR(light.getPosition(BodyLock::LOCK), expectedLight, errorLight);
    EXPECT_VEC3_NEAR(heavy.getPosition(BodyLock::LOCK), expectedHeavy, errorHeavy);
}