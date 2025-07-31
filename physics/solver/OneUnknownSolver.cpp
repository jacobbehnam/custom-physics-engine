#include "OneUnknownSolver.h"

#include <glm/vec3.hpp>

template<typename InputT, typename OutputT>
OneUnknownSolver<InputT, OutputT>::OneUnknownSolver(ParamSetter setParameter, SimulationRun runSimulation, ResultExtractor extractResult, OutputT tgt, OutputT toler, int maxIter)
    : setParam(std::move(setParameter)), runSim(std::move(runSimulation)), extract(std::move(extractResult)), target(tgt), tolerance(toler), maxIterations(maxIter) {}

// Secant solve method
template<typename InputT, typename OutputT>
InputT OneUnknownSolver<InputT, OutputT>::solve(InputT x0, InputT x1) {
    OutputT f0 = eval(x0);
    OutputT f1 = eval(x1);

    for (iteration = 0; iteration < maxIterations; ++iteration) {
        if (std::abs(f1) < tolerance)
            return x1;

        if (std::abs(f1 - f0) < std::numeric_limits<OutputT>::epsilon())
            break;

        // Secant method step
        InputT x2 = x1 - (f1 * (x1 - x0) / (f1 - f0));
        OutputT f2 = eval(x2);

        x0 = x1;
        f0 = f1;
        x1 = x2;
        f1 = f2;
    }
    return x1;
}

template<typename InputT, typename OutputT>
OutputT OneUnknownSolver<InputT, OutputT>::eval(InputT x) {
    setParam(x);
    runSim();
    OutputT result = extract();
    lastError = result - target;
    return lastError;
}

template class OneUnknownSolver<float, float>;