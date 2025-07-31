#pragma once

#include <functional>
#include <limits>

template<typename InputT, typename OutputT>
class OneUnknownSolver {
public:
    using ParamSetter = std::function<void(InputT)>; // Sets the value for the unknown parameter we are solving for
    using SimulationRun = std::function<void()>; // Runs the simulation up until a stop condition is reached
    using ResultExtractor = std::function<OutputT()>; // Gets a value to compare to the target value after an iteration

    OneUnknownSolver(ParamSetter setParameter,
                     SimulationRun runSimulation,
                     ResultExtractor extractResult,
                     OutputT target,
                     OutputT tolerance = static_cast<OutputT>(1e-3),
                     int maxIterations = 30);

    InputT solve(InputT x0, InputT x1);

    OutputT getLastError() const { return lastError; }
    int getIterations() const { return iteration; }

private:
    OutputT eval(InputT x);

    ParamSetter setParam;
    SimulationRun runSim;
    ResultExtractor extract;

    OutputT target;
    OutputT tolerance;
    int maxIterations;

    OutputT lastError = std::numeric_limits<OutputT>::max();
    int iteration = 0;
};