#pragma once
#include <string>
#include <memory>
#include "OneUnknownSolver.h"
#include "VectorRootSolver.h"

namespace Physics {
    class PhysicsSystem;
    class PhysicsBody;
}

enum class SolverMode {
    SIMULATE, // no unknowns to solve for, just advance sim
    SOLVE // unknown must be solved using VectorRootSolver
};

struct SolverDecision {
    SolverMode mode;
    std::unique_ptr<VectorRootSolver<glm::vec3, glm::vec3>> solver; // nullptr if direct simulation
};

class ProblemRouter {
public:
    explicit ProblemRouter(Physics::PhysicsSystem& physicsSystem);
    std::unique_ptr<VectorRootSolver<glm::vec3, glm::vec3>> makeSolver(Physics::PhysicsBody* body, const std::unordered_map<std::string, double> &knowns, const std::string &unknown);
    SolverDecision routeProblem(Physics::PhysicsBody* body, const std::unordered_map<std::string,double>& knowns, const std::string& unknown);

    std::vector<std::vector<std::string>> getRequiredKeys(const std::string& unknown) const;
private:
    Physics::PhysicsSystem& physicsSystem;

    // Takes in knowns and returns a solver.
    using SolverFactory = std::function<std::unique_ptr<VectorRootSolver<glm::vec3, glm::vec3>>(Physics::PhysicsBody* body, const std::unordered_map<std::string,double>&)>;

    struct SolverEntry {
        std::vector<std::string> requiredKeys;
        SolverFactory factory;
    };

    bool areRequirementsMet(const std::vector<std::string>& required, const std::unordered_map<std::string, double>& knowns) const;

    // Maps an unknown to solve for to a vector of potential SolverFactories
    // Can refactor the vector of SolverFactories to a tree later
    std::unordered_map<std::string, std::vector<SolverEntry>> solverMap;
    void registerKinematicsProblems();
};
