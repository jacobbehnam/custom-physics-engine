#include "OneUnknownSolver.h"

#include <iostream>
#include <glm/vec3.hpp>

template<typename InputT, typename OutputT>
OneUnknownSolver<InputT, OutputT>::OneUnknownSolver(ParamSetter setParameter, SimulationRun runSimulation, ResultExtractor extractResult, OutputT tgt, OutputT toler, int maxIter)
    : setParam(std::move(setParameter)), runSim(std::move(runSimulation)), extract(std::move(extractResult)), target(tgt), tolerance(toler), current(static_cast<InputT>(0)) {
    setParam(current);
}

template<typename InputT, typename OutputT>
bool OneUnknownSolver<InputT, OutputT>::stepFrame() {
    if (runSim()) {
        // if stop condition is reached
        OutputT measured = extract();
        OutputT error    = measured - target;

        if (intervalFound) {
            std::cout << error << std::endl;
            if (std::abs(error) < tolerance) {
                return true;
            }

            if (error > 0) {
                // Measured > target → current guess too low
                low = current;
            } else {
                // Measured < target → current guess too high
                high = current;
            }

            // Bisection step (midpoint)
            current = static_cast<InputT>((low + high) * static_cast<OutputT>(0.5));
            setParam(current); // Apply next guess
        } else { // Work on finding interval
            if (prevError != 0 && expandIterations % 2 == 0 && prevError * error <= 0) {
                intervalFound = true;
                current = high + low * static_cast<OutputT>(0.5);
                setParam(current);
                return false;
            }
            if (expandIterations % 2 == 0) {
                delta *= 2;
                low -= delta;
                current = low;
            } else {
                high += delta;
                current = high;
            }
            expandIterations++;
            setParam(current);
        }
        prevError = error;
    }
    return false;
}

template class OneUnknownSolver<float, float>;