#pragma once
#include <functional>

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
                     int maxIterations = 30);

    bool stepFrame();

    InputT current;   // current guess for unknown(s)

private:
    InitialGuessSetter setGuess;
    StopCondition stopCondition;
    ResultExtractor extract;
    OutputT target;
    double tolerance;
    //int maxIterations;
};