#pragma once
#include <functional>
#include <glm/glm.hpp>

template<typename InputT, typename OutputT>
class VectorRootSolver {
public:
    using InitialGuessSetter = std::function<void(const InputT&)>; // Sets the value for the unknown parameter we are solving for
    using StopCondition = std::function<bool()>; // Runs the simulation up until a stop condition is reached
    using ResultExtractor = std::function<OutputT()>; // Gets a value to compare to the target value after an iteration

    VectorRootSolver(InitialGuessSetter initialGuessSetter,
                     StopCondition stopCondition,
                     ResultExtractor extractResult,
                     const OutputT& target,
                     double tolerance = 1e-3,
                     int maxIterations = 30,
                     double jacobianStep = 0.01,
                     double damping = 1.0);

    bool stepFrame();

    InputT current;   // current guess for unknown(s)

private:
    enum class NewtonState { WaitingForBase, PerturbComponent, WaitingForPerturbed, ComputeJacobian };
    NewtonState state;

    InitialGuessSetter setGuess;
    StopCondition stopCondition;
    ResultExtractor extract;

    glm::vec3 target;
    glm::vec3 baseOutput;
    glm::vec3 xPerturbed;
    std::array<glm::vec3, 3> fPerturbed;

    int currentPerturbation = -1;
    float h;
    float tolerance;
    float alpha;
};