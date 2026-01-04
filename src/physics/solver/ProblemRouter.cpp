#include "ProblemRouter.h"
#include <iostream>
#include <set>

#include "InterceptSolver.h"
#include "physics/PhysicsSystem.h"

ProblemRouter::ProblemRouter(Physics::PhysicsSystem& physics) : physicsSystem(physics) {
    registerKinematicsProblems();
}

SolverDecision ProblemRouter::routeProblem(Physics::PhysicsBody* body, const std::unordered_map<std::string,double>& knowns, const std::string& unknown) const {
    // Case 1: We know all initial conditions
    if (knowns.find("r0") != knowns.end() && knowns.find("v0") != knowns.end()) {
        return {SolverMode::SIMULATE, nullptr}; // TODO
    }

    // Case 2: Unknown requires solving
    auto solver = makeSolver(body, knowns, unknown);
    return {SolverMode::SOLVE, std::move(solver)};
}

std::vector<std::vector<std::string>> ProblemRouter::getRequiredKeys(const std::string &unknown) const {
    std::vector<std::vector<std::string>> options;

    auto it = solverMap.find(unknown);
    if (it != solverMap.end()) {
        // Iterate over all registered ways to solve this problem
        for (const auto& entry : it->second) {
            options.push_back(entry.requiredKeys);
        }
    }
    return options;
}

bool ProblemRouter::areRequirementsMet(const std::vector<std::string>& required, const std::unordered_map<std::string, double>& knowns) const {
    return std::all_of(required.begin(), required.end(), [&](const std::string& key) {
            return knowns.find(key) != knowns.end();
        });
}

std::unique_ptr<ISolver> ProblemRouter::makeSolver(Physics::PhysicsBody* body, const std::unordered_map<std::string, double> &knowns, const std::string &unknown) const {
    auto it = solverMap.find(unknown);
    if (it == solverMap.end()) {
        std::cerr << "No solver registered for unknown: " << unknown << std::endl;
        return nullptr;
    }

    for (const auto& entry : it->second) {
        if (areRequirementsMet(entry.requiredKeys, knowns)) {
            return entry.factory(body, knowns);
        }
    }

    std::cerr << "Insufficient knowns provided for unknown: " << unknown << std::endl;
    return nullptr;
}

void ProblemRouter::registerKinematicsProblems() {
    constexpr double maxSimTime = 10.0; // Seconds

    // SolverEntry v0Entry;
    // v0Entry.requiredKeys = {"r0_x","r0_y","r0_z","rT_x","rT_y","rT_z", "T"};
    // v0Entry.factory = [this](Physics::PhysicsBody* body, const std::unordered_map<std::string,double>& knowns) {
    //     glm::vec3 r0(knowns.at("r0_x"), knowns.at("r0_y"), knowns.at("r0_z"));
    //     glm::vec3 rT(knowns.at("rT_x"), knowns.at("rT_y"), knowns.at("rT_z"));
    //     double T = knowns.at("T");
    //
    //     auto setter = [=](const glm::vec3& v0){
    //         physicsSystem.reset();
    //         body->setVelocity(v0, BodyLock::LOCK);
    //     };
    //     auto stopCondition = [=]() -> bool {
    //         const auto& frames = body->getAllFrames(BodyLock::LOCK);
    //         if (frames.empty()) return false;
    //
    //         // Wait until maxSimTime reached
    //         if (physicsSystem.simTime >= T) return true;
    //
    //         // Also stop if the object is effectively at rest
    //         const glm::vec3& v = frames.back().velocity;
    //         if (glm::length(v) < 1e-3f) return true;
    //
    //         return false;
    //     };
    //     auto extractVector = [=]() -> glm::vec3 {
    //         const auto& frames = body->getAllFrames(BodyLock::LOCK);
    //         if (frames.empty()) return glm::vec3(0.0f);
    //
    //         // Take the last frame's position as the output
    //         return frames.back().position;
    //     };
    //
    //     glm::vec3 target = rT;
    //
    //     return std::make_unique<VectorRootSolver<glm::vec3, glm::vec3>>(
    //         setter, stopCondition, extractVector, target,
    //         1e-3, 30, 0.01, 1.0
    //     );
    // };
    // solverMap["v0"].push_back(v0Entry);
    //
    // SolverEntry tEntry;
    // tEntry.requiredKeys = {"r0_x","r0_y","r0_z", "v0_x","v0_y","v0_z", "rT_x","rT_y","rT_z"};
    // tEntry.factory = [this](Physics::PhysicsBody* body, const std::unordered_map<std::string,double>& knowns) {
    //     glm::vec3 r0(knowns.at("r0_x"), knowns.at("r0_y"), knowns.at("r0_z"));
    //     glm::vec3 v0(knowns.at("v0_x"), knowns.at("v0_y"), knowns.at("v0_z"));
    //     glm::vec3 rT(knowns.at("rT_x"), knowns.at("rT_y"), knowns.at("rT_z"));
    //
    //     physicsSystem.reset();
    //     body->setPosition(r0, BodyLock::LOCK);
    //     body->setVelocity(v0, BodyLock::LOCK);
    //
    //     auto monitor = [=]() -> float {
    //         glm::vec3 currentPos = body->getPosition(BodyLock::LOCK);
    //         float dist = glm::distance(currentPos, rT);
    //
    //         std::cout << dist << std::endl;
    //         if (dist < 1e-3) return -1.0f;
    //
    //         glm::vec3 currentVel = body->getVelocity(BodyLock::LOCK);
    //         glm::vec3 toTarget = rT - currentPos;
    //
    //         return glm::dot(currentVel, toTarget);
    //     };
    //
    //     auto timeout = [=]() -> bool {
    //         return physicsSystem.simTime > 60.0f;
    //     };
    //
    //     return std::make_unique<InterceptSolver>(monitor, timeout);
    // };
    // solverMap["T"].push_back(tEntry);

    // TODO: make a helper and make an enum or something for the required keys
    SolverEntry eventEntry;
    eventEntry.requiredKeys = {
        "r0_x", "r0_y", "r0_z",
        "v0_x", "v0_y", "v0_z",
        "Stop_SubjectID", "Stop_Prop", "Stop_Op", "Stop_Val",
        "Stop_TargetID",
        "Stop_Val_X", "Stop_Val_Y", "Stop_Val_Z"
    };

    eventEntry.factory = [this](Physics::PhysicsBody* body, const std::unordered_map<std::string, double>& knowns) {
        glm::vec3 r0(knowns.at("r0_x"), knowns.at("r0_y"), knowns.at("r0_z"));
        glm::vec3 v0(knowns.at("v0_x"), knowns.at("v0_y"), knowns.at("v0_z"));

        physicsSystem.reset();
        body->setPosition(r0, BodyLock::LOCK);
        body->setVelocity(v0, BodyLock::LOCK);

        int subjectID = (int)knowns.at("Stop_SubjectID");
        int prop      = (int)knowns.at("Stop_Prop");
        int op        = (int)knowns.at("Stop_Op");
        float val     = (float)knowns.at("Stop_Val");

        if (subjectID == -1) {
            std::cout << "Solver Error: No subject selected." << std::endl;
            auto dummyMonitor = []() { return -1.0f; };
            auto dummyTimeout = []() { return true; };
            return std::make_unique<InterceptSolver>(dummyMonitor, dummyTimeout);
        }

        Physics::PhysicsBody* subject = physicsSystem.getBodyById(subjectID);
        if (!subject) { /* Handle missing subject safely if needed */ }

        int targetID = (int)knowns.at("Stop_TargetID");
        glm::vec3 targetPoint(knowns.at("Stop_Val_X"), knowns.at("Stop_Val_Y"), knowns.at("Stop_Val_Z"));
        Physics::PhysicsBody* targetBody = physicsSystem.getBodyById(targetID);

        auto monitor = [=]() -> float {
            float currentVal = 0.0f;

            switch (prop) {
            case 0: // Pos Y
                currentVal = subject->getPosition(BodyLock::LOCK).y;
                break;

            case 1: // Vel Y
                currentVal = subject->getVelocity(BodyLock::LOCK).y;
                break;

            case 2: // Distance to Object
            {
                if (targetBody) {
                    auto getClosestOnBody = [&](Physics::PhysicsBody* b, const glm::vec3& targetPos) -> glm::vec3 {
                        if (auto* col = b->getCollider()) {
                            return col->closestPoint(targetPos).point;
                        } else {
                            return b->getPosition(BodyLock::LOCK);
                        }
                    };

                    glm::vec3 centerSubject = subject->getPosition(BodyLock::LOCK);
                    glm::vec3 pTarget = getClosestOnBody(targetBody, centerSubject);
                    glm::vec3 pSubject = getClosestOnBody(subject, pTarget);

                    currentVal = glm::distance(pSubject, pTarget);
                } else {
                    currentVal = 99999.0f;
                }
                break;
            }

            case 3: // Distance to Point
                currentVal = glm::distance(subject->getPosition(BodyLock::LOCK), targetPoint);
                break;

            case 4: // Time
                currentVal = physicsSystem.simTime;
                break;
            }

            if (op == 0) {
                return (currentVal - val);
            }
            else {
                return (val - currentVal);
            }
        };

        auto timeout = [=]() { return false; }; // TODO: temporary no timeout

        return std::make_unique<InterceptSolver>(monitor, timeout);
    };
    solverMap["Event"].push_back(eventEntry);

    SolverEntry v0Entry;
    v0Entry.requiredKeys = {
        "r0_x", "r0_y", "r0_z",
        "Stop_SubjectID", "Stop_Prop", "Stop_Op", "Stop_Val",
        "Stop_TargetID",
        "Stop_Val_X", "Stop_Val_Y", "Stop_Val_Z",
        "Target_Time"
    };

    v0Entry.factory = [this](Physics::PhysicsBody* body, const std::unordered_map<std::string, double>& knowns) {
        int subjectID = (int)knowns.at("Stop_SubjectID");
        int stopTargetID = (int)knowns.at("Stop_TargetID");

        double targetTime = knowns.count("Target_Time") ? knowns.at("Target_Time") : -1.0;

        Physics::PhysicsBody* subjectBody = physicsSystem.getBodyById(subjectID);
        Physics::PhysicsBody* targetBody = physicsSystem.getBodyById(stopTargetID);

        glm::vec3 r0(knowns.at("r0_x"), knowns.at("r0_y"), knowns.at("r0_z"));
        int prop = (int)knowns.at("Stop_Prop");
        int op = (int)knowns.at("Stop_Op");
        float val = (float)knowns.at("Stop_Val");
        glm::vec3 stopTargetPoint(knowns.at("Stop_Val_X"), knowns.at("Stop_Val_Y"), knowns.at("Stop_Val_Z"));

        glm::vec3 targetPos(0.0f);
        if (prop == 3) {
            targetPos = stopTargetPoint;
        } else if (prop == 2 && targetBody) {
            targetPos = targetBody->getPosition(BodyLock::LOCK);
        }

        auto setter = [=](const glm::vec3& guess_v0) {
            physicsSystem.reset();
            body->setVelocity(guess_v0, BodyLock::LOCK);
        };

        auto stopCondition = [=]() -> bool {
            if (targetTime > 0.0f) {
                return physicsSystem.simTime >= targetTime;
            }
            if (!subjectBody) return true; // Safety check

            float currentVal = 0.0f;
            switch (prop) {
            case 0:
                currentVal = subjectBody->getPosition(BodyLock::LOCK).y;
                break;
            case 1:
                currentVal = subjectBody->getVelocity(BodyLock::LOCK).y;
                break;
            case 2:
                if (targetBody) {
                    currentVal = glm::distance(subjectBody->getPosition(BodyLock::LOCK),
                                               targetBody->getPosition(BodyLock::LOCK));
                } else {
                    currentVal = 99999.0f;
                }
                break;
            case 3:
                currentVal = glm::distance(subjectBody->getPosition(BodyLock::LOCK), stopTargetPoint);
                break;
            }

            if (op == 0) return (currentVal <= val);
            else return (currentVal >= val);
        };

        auto extractor = [=]() -> glm::vec3 {
            return body->getPosition(BodyLock::LOCK);
        };

        return std::make_unique<VectorRootSolver<glm::vec3, glm::vec3>>(
            setter, stopCondition, extractor, targetPos,
            1e-3, 30, 0.01, 1.0
        );
    };

    solverMap["v0"].push_back(v0Entry);
}
