#include <gtest/gtest.h>
#include <cmath>
#include "physics/PhysicsSystem.h"
#include "physics/PointMass.h"
#include "physics/RigidBody.h"
#include "physics/bounding/BoxCollider.h"
#include "physics/utils/ThermalUtils.h"

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

    EXPECT_FLOAT_EQ(system.getGlobalAcceleration().y, -Constants::STANDARD_GRAVITY);
    EXPECT_FLOAT_EQ(system.getSimSpeed(), 1.0f);

    system.setGlobalAcceleration(glm::vec3(0.0f));
    EXPECT_VEC3_EXACT(system.getGlobalAcceleration(), glm::vec3(0.0f, 0.0f, 0.0f));

    system.setSimSpeed(0.5f);
    EXPECT_FLOAT_EQ(system.getSimSpeed(), 0.5f);
}

TEST(PhysicsSystem, Step_OverlappingBodies_DoesNotCrash) {
    Physics::PhysicsSystem system(glm::vec3(0.0f));
    Physics::PointMass a(0, 1.0e6);
    Physics::PointMass b(1, 1.0e6);

    a.setPosition(glm::vec3(0.0f), BodyLock::LOCK);
    b.setPosition(glm::vec3(0.0f), BodyLock::LOCK);
    system.addBody(&a);
    system.addBody(&b);

    ASSERT_NO_FATAL_FAILURE(system.step(0.001f));
    EXPECT_TRUE(std::isfinite(a.getPosition(BodyLock::LOCK).x));
    EXPECT_TRUE(std::isfinite(b.getPosition(BodyLock::LOCK).x));
}

TEST(PhysicsSystem, SampleSolverGravity_DoesNotDriftSideways) {
    Physics::PhysicsSystem system(glm::vec3(0.0f, -Constants::STANDARD_GRAVITY, 0.0f));
    system.setGravitationalConstant(Constants::G);

    Physics::PointMass ball(0, 1.0, glm::vec3(0.0f, 0.0f, 0.0f), false);
    ball.setVelocity(glm::vec3(0.0f, 15.0f, 0.0f), BodyLock::LOCK);

    Physics::PointMass keys(1, 1.0, glm::vec3(0.0f, 20.0f, 0.0f), false);

    system.addBody(&ball);
    system.addBody(&keys);

    constexpr float dt = 1.0f / 120.0f;
    for (int i = 0; i < 120; ++i) {
        system.step(dt);
    }

    EXPECT_NEAR(ball.getPosition(BodyLock::LOCK).x, 0.0f, 1.0e-6f);
    EXPECT_NEAR(ball.getPosition(BodyLock::LOCK).z, 0.0f, 1.0e-6f);
    EXPECT_NEAR(keys.getPosition(BodyLock::LOCK).x, 0.0f, 1.0e-6f);
    EXPECT_NEAR(keys.getPosition(BodyLock::LOCK).z, 0.0f, 1.0e-6f);
}

TEST(ThermalUtils, ConductiveExchange_ConservesEnergyAndDoesNotOvershoot) {
    ThermalProperties hot;
    hot.tempK = 400.0;
    hot.specificHeat = 1000.0f;
    hot.conductivity = 1000.0f;

    ThermalProperties cold;
    cold.tempK = 300.0;
    cold.specificHeat = 1000.0f;
    cold.conductivity = 1000.0f;

    const double massHot = 1.0;
    const double massCold = 1.0;
    const double initialEnergy = massHot * hot.specificHeat * hot.tempK + massCold * cold.specificHeat * cold.tempK;

    Physics::Thermal::applyConductiveExchange(hot, massHot, cold, massCold, 1.0, 0.01, 1000.0);

    const double finalEnergy = massHot * hot.specificHeat * hot.tempK + massCold * cold.specificHeat * cold.tempK;
    EXPECT_NEAR(finalEnergy, initialEnergy, 1.0e-6);
    EXPECT_DOUBLE_EQ(hot.tempK, 350.0);
    EXPECT_DOUBLE_EQ(cold.tempK, 350.0);
}

TEST(ThermalUtils, AmbientRadiation_UsesStefanBoltzmannSignConvention) {
    ThermalProperties props;
    props.tempK = 400.0;
    props.emissivity = 1.0f;

    const double cooling = Physics::Thermal::ambientRadiationHeatRate(props, 1.0, 300.0);
    const double heating = Physics::Thermal::ambientRadiationHeatRate(props, 1.0, 500.0);

    EXPECT_LT(cooling, 0.0);
    EXPECT_GT(heating, 0.0);
}

TEST(ThermalUtils, HeatCapacity_UsesThermalMassFraction) {
    ThermalProperties props;
    props.specificHeat = 1000.0f;
    props.thermalMassFraction = 0.25f;

    EXPECT_DOUBLE_EQ(Physics::Thermal::heatCapacity(8.0, props), 2000.0);
}

TEST(ThermalUtils, ExternalHeatFluxRate_UsesSurfaceArea) {
    ThermalProperties props;
    props.externalHeatFlux = 250.0;

    EXPECT_DOUBLE_EQ(Physics::Thermal::externalHeatFluxRate(props, 4.0), 1000.0);
}

TEST(ThermalUtils, EffectiveProperties_UseTemperatureCoefficients) {
    ThermalProperties props;
    props.referenceTempK = 300.0f;
    props.specificHeat = 1000.0f;
    props.specificHeatTempCoeff = 0.01f;
    props.conductivity = 10.0f;
    props.conductivityTempCoeff = -0.01f;
    props.density = 1000.0f;
    props.linearExpansionCoeff = 1.0e-4f;

    EXPECT_NEAR(Physics::Thermal::effectiveSpecificHeat(props, 310.0), 1100.0, 1.0e-4);
    EXPECT_NEAR(Physics::Thermal::effectiveConductivity(props, 310.0), 9.0, 1.0e-5);
    EXPECT_NEAR(Physics::Thermal::effectiveDensity(props, 310.0), 997.008973, 1.0e-5);
}

TEST(ThermalUtils, CarnotEfficiency_UsesAbsoluteTemperatures) {
    EXPECT_DOUBLE_EQ(Physics::Thermal::carnotEfficiency(600.0, 300.0), 0.5);
    EXPECT_DOUBLE_EQ(Physics::Thermal::carnotEfficiency(300.0, 600.0), 0.0);
}

TEST(ThermalUtils, ApplyThermalEnergy_ConsumesLatentHeatAtMeltingPoint) {
    ThermalProperties props;
    props.tempK = 300.0;
    props.specificHeat = 100.0f;
    props.meltingPoint = 310.0f;
    props.latentHeatFusion = 1000.0f;

    Physics::Thermal::applyThermalEnergy(props, 1.0, 1500.0);
    EXPECT_DOUBLE_EQ(props.tempK, 310.0);
    EXPECT_FLOAT_EQ(props.fusionProgress, 0.5f);
    EXPECT_GT(props.entropyJPerK, 0.0);

    Physics::Thermal::applyThermalEnergy(props, 1.0, 500.0);
    EXPECT_DOUBLE_EQ(props.tempK, 310.0);
    EXPECT_FLOAT_EQ(props.fusionProgress, 1.0f);

    Physics::Thermal::applyThermalEnergy(props, 1.0, 100.0);
    EXPECT_DOUBLE_EQ(props.tempK, 311.0);
}

TEST(PhysicsSystem, Step_StaticBody_UpdatesTemperatureButNotPosition) {
    Physics::PhysicsSystem system(glm::vec3(0.0f));
    system.setAmbientTemperature(300.0f);

    Physics::PointMass pm(0, 10.0, glm::vec3(1.0f, 2.0f, 3.0f), true);
    ThermalProperties props;
    props.tempK = 400.0;
    props.specificHeat = 1000.0f;
    props.heatTransferCoeff = 10.0f;
    props.emissivity = 0.0f;
    props.density = 1000.0f;
    pm.setThermalProperty(props, BodyLock::LOCK);

    system.addBody(&pm);
    system.step(10.0f);

    EXPECT_VEC3_EXACT(pm.getPosition(BodyLock::LOCK), glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_LT(pm.getThermalProperties(BodyLock::LOCK).tempK, 400.0);
}

TEST(PhysicsSystem, Step_InternalHeatPower_ChangesTemperature) {
    Physics::PhysicsSystem system(glm::vec3(0.0f));
    Physics::PointMass pm(0, 10.0, glm::vec3(0.0f), true);

    ThermalProperties props;
    props.tempK = 300.0;
    props.specificHeat = 1000.0f;
    props.heatTransferCoeff = 0.0f;
    props.emissivity = 0.0f;
    props.internalHeatPower = 1000.0;
    pm.setThermalProperty(props, BodyLock::LOCK);

    system.addBody(&pm);
    system.step(10.0f);

    EXPECT_NEAR(pm.getThermalProperties(BodyLock::LOCK).tempK, 301.0, 1.0e-6);
}

TEST(PhysicsBody, LoadFrame_RestoresTemperature) {
    Physics::PointMass pm(0, 1.0);
    ThermalProperties props;
    props.tempK = 500.0;
    pm.setThermalProperty(props, BodyLock::LOCK);

    ObjectSnapshot snapshot{&pm, 0.0f, glm::vec3(1.0f), glm::vec3(2.0f), 275.0f};
    pm.loadFrame(snapshot, BodyLock::LOCK);

    EXPECT_FLOAT_EQ(pm.getThermalProperties(BodyLock::LOCK).tempK, 275.0f);
}

TEST(PhysicsBody, VisibleEmission_IgnoresRoomTemperature) {
    Physics::PointMass pm(0, 1.0);
    ThermalProperties props;
    props.tempK = 293.15;
    props.emissivity = 1.0f;
    pm.setThermalProperty(props, BodyLock::LOCK);

    EXPECT_VEC3_EXACT(pm.getEmission(BodyLock::LOCK), glm::vec3(0.0f));
}

TEST(PhysicsBody, VisibleEmission_HotBodiesEmitLight) {
    Physics::PointMass pm(0, 1.0);
    ThermalProperties props;
    props.tempK = 1800.0;
    props.emissivity = 1.0f;
    pm.setThermalProperty(props, BodyLock::LOCK);

    const glm::vec3 emission = pm.getEmission(BodyLock::LOCK);
    EXPECT_GT(emission.r, 0.0f);
    EXPECT_GE(emission.r, emission.g);
    EXPECT_GE(emission.g, emission.b);
}

TEST(RigidBody, SurfaceArea_UsesAllScaleAxes) {
    auto collider = std::make_unique<Physics::Bounding::BoxCollider>(
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
    );
    Physics::RigidBody body(0, 1.0, std::move(collider));

    std::vector<glm::vec3> vertices = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
    };
    std::vector<unsigned int> indices = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        0, 4, 5, 0, 5, 1,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        0, 3, 7, 0, 7, 4,
    };

    body.setGeometry(vertices, indices);
    body.setScale(glm::vec3(2.0f, 3.0f, 4.0f));

    EXPECT_NEAR(body.getSurfaceArea(), 52.0f, 1.0e-5f);
}

TEST(RigidBody, CollisionHeat_ZeroSpecificHeat_DoesNotCreateNaN) {
    auto collider = std::make_unique<Physics::Bounding::BoxCollider>(
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
    );
    Physics::RigidBody body(0, 1.0, std::move(collider), glm::vec3(0.0f), true);
    Physics::PointMass pm(1, 1.0, glm::vec3(0.0f), false);
    pm.setVelocity(glm::vec3(0.0f, -1.0f, 0.0f), BodyLock::LOCK);

    ThermalProperties bodyProps;
    bodyProps.specificHeat = 0.0f;
    body.setThermalProperty(bodyProps, BodyLock::LOCK);

    ThermalProperties pmProps;
    pmProps.specificHeat = 0.0f;
    pm.setThermalProperty(pmProps, BodyLock::LOCK);

    body.resolveCollisionWithPointMass(0.01f, pm);

    EXPECT_TRUE(std::isfinite(body.getThermalProperties(BodyLock::LOCK).tempK));
    EXPECT_TRUE(std::isfinite(pm.getThermalProperties(BodyLock::LOCK).tempK));
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
    constexpr float gravity = -Constants::STANDARD_GRAVITY;
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
    constexpr float gravity = -Constants::STANDARD_GRAVITY;
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
    constexpr float gravity = -Constants::STANDARD_GRAVITY;
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
