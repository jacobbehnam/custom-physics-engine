#include <gtest/gtest.h>
#include "physics/PhysicsSystem.h"

TEST(SanityCheck, BasicMath) {
    EXPECT_EQ(2 + 2, 4);
}

TEST(PhysicsCore, CanCreateSystem) {
    Physics::PhysicsSystem system;
    system.setGlobalAcceleration({0, -9.81, 0});
    EXPECT_EQ(system.getGlobalAcceleration().y, -9.81f);
}