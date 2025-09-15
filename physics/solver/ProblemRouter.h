#pragma once
#include <string>
#include <memory>
#include "OneUnknownSolver.h"

class ProblemRouter {
public:
    ProblemRouter() = default;
    std::unique_ptr<OneUnknownSolver<double, double>> makeSolver(const std::unordered_map<std::string, double>& knowns, const std::string& unknown = "");

private:
    // Takes in knowns and returns a solver.
    using SolverFactory = std::function<std::unique_ptr<OneUnknownSolver<double, double>>(const std::unordered_map<std::string,double>&)>;

    struct SolverEntry {
        std::vector<std::string> requiredKeys;
        SolverFactory factory;
    };

    // Maps an unknown to solve for to a vector of potential SolverFactories
    // Can refactor the vector of SolverFactories to a tree later
    std::unordered_map<std::string, std::vector<SolverEntry>> solverMap;
    void registerKinematicsProblems();
};
