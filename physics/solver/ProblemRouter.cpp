#include "ProblemRouter.h"
#include <iostream>
#include <set>

std::unique_ptr<OneUnknownSolver<double, double>> ProblemRouter::makeSolver(const std::unordered_map<std::string, double> &knowns, const std::string &unknown) {
    auto it = solverMap.find(unknown);
    if (it == solverMap.end()) {
        std::cerr << "No solver registered for unknown: " << unknown << std::endl;
        return nullptr;
    }

    std::set<std::string> userKeys;
    for (const auto& pair : knowns)
        userKeys.insert(pair.first);

    // Iterate through factories
    for (const auto& entry : it->second) {
        std::set<std::string> requiredKeys(entry.requiredKeys.begin(), entry.requiredKeys.end());

        if (userKeys == requiredKeys) {
            // TODO: Make this work with partial matches too
            return entry.factory(knowns);
        }
    }

    std::cerr << "No exact matching solver found for unknown: " << unknown << std::endl;
    return nullptr;
}
