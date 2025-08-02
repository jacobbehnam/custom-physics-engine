#pragma once

#include <functional>
#include <limits>

struct ObjectData;

template<typename InputT, typename OutputT>
class OneUnknownSolver {
public:
    using ParamSetter = std::function<void(InputT)>; // Sets the value for the unknown parameter we are solving for
    using SimulationRun = std::function<bool()>; // Runs the simulation up until a stop condition is reached
    using ResultExtractor = std::function<OutputT()>; // Gets a value to compare to the target value after an iteration

    OneUnknownSolver(ParamSetter setParameter,
                     SimulationRun runSimulation,
                     ResultExtractor extractResult,
                     OutputT target,
                     OutputT tolerance = static_cast<OutputT>(0.001),
                     int maxIterations = 30);

    bool stepFrame();

    InputT current;
private:
    bool intervalFound = false;
    float delta = 0.5f;
    int expandIterations = 0;
    OutputT prevError{};

    ParamSetter setParam;
    SimulationRun runSim;
    ResultExtractor extract;

    OutputT target;
    InputT high{};
    InputT low{};
    OutputT tolerance;
};