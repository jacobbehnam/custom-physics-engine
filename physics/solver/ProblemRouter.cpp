#include "ProblemRouter.h"
#include <iostream>
#include <set>
#include "physics/PhysicsSystem.h"

ProblemRouter::ProblemRouter(Physics::PhysicsSystem& physics) : physicsSystem(physics) {
    registerKinematicsProblems();
}

SolverDecision ProblemRouter::routeProblem(Physics::PhysicsBody* body, const std::unordered_map<std::string,double>& knowns, const std::string& unknown) {
    // Case 1: We know all initial conditions
    if (knowns.find("r0") != knowns.end() && knowns.find("v0") != knowns.end()) {
        return {SolverMode::SIMULATE, nullptr}; // TODO
    }

    // Case 2: Unknown requires solving
    auto solver = makeSolver(body, knowns, unknown);
    return {SolverMode::SOLVE, std::move(solver)};
}

std::unique_ptr<VectorRootSolver<glm::vec3, glm::vec3>> ProblemRouter::makeSolver(Physics::PhysicsBody* body, const std::unordered_map<std::string, double> &knowns, const std::string &unknown) {
    auto it = solverMap.find(unknown);
    if (it == solverMap.end()) {
        std::cerr << "No solver registered for unknown: " << unknown << std::endl;
        return nullptr;
    }

    std::set<std::string> userKeys;
    for (const auto& pair : knowns)
        userKeys.insert(pair.first);

    // Iterate through factories that solve for the unknown
    for (const auto& entry : it->second) {
        bool hasAllKeys = true;
        for (const auto& key : entry.requiredKeys) {
            if (knowns.find(key) == knowns.end()) {
                hasAllKeys = false;
                break;
            }
        }
        if (hasAllKeys) {
            return entry.factory(body, knowns);
        }
    }

    std::cerr << "No exact matching solver found for unknown: " << unknown << std::endl;
    return nullptr;
}

void ProblemRouter::registerKinematicsProblems() {
    constexpr double maxSimTime = 10.0; // Seconds

    SolverEntry v0Entry;
    v0Entry.requiredKeys = {"r0_x","r0_y","r0_z","rT_x","rT_y","rT_z", "T"};
    v0Entry.factory = [this](Physics::PhysicsBody* body, const std::unordered_map<std::string,double>& knowns) {
        glm::vec3 r0(knowns.at("r0_x"), knowns.at("r0_y"), knowns.at("r0_z"));
        glm::vec3 rT(knowns.at("rT_x"), knowns.at("rT_y"), knowns.at("rT_z"));
        double T = knowns.at("T");

        auto setter = [=](const glm::vec3& v0){
            physicsSystem.reset();
            body->setVelocity(v0, BodyLock::LOCK);
        };
        auto stopCondition = [=]() -> bool {
            const auto& frames = body->getAllFrames(BodyLock::LOCK);
            if (frames.empty()) return false;

            // Wait until maxSimTime reached
            if (physicsSystem.simTime >= T) return true;

            // Also stop if the object is effectively at rest
            const glm::vec3& v = frames.back().velocity;
            if (glm::length(v) < 1e-3f) return true;

            return false;
        };
        auto extractVector = [=]() -> glm::vec3 {
            const auto& frames = body->getAllFrames(BodyLock::LOCK);
            if (frames.empty()) return glm::vec3(0.0f);

            // Take the last frame's position as the output
            return frames.back().position;
        };

        glm::vec3 target = rT;

        return std::make_unique<VectorRootSolver<glm::vec3, glm::vec3>>(
            setter, stopCondition, extractVector, target,
            1e-3, 30, 0.01, 1.0
        );
    };
    solverMap["v0"].push_back(v0Entry);
}
