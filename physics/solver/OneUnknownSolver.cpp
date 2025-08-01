#include "OneUnknownSolver.h"

#include <glm/vec3.hpp>

template<typename InputT, typename OutputT>
OneUnknownSolver<InputT, OutputT>::OneUnknownSolver(ParamSetter setParameter, SimulationRun runSimulation, ResultExtractor extractResult, OutputT tgt, OutputT toler, int maxIter)
    : setParam(std::move(setParameter)), runSim(std::move(runSimulation)), extract(std::move(extractResult)), target(tgt), tolerance(toler) {}

template<typename InputT, typename OutputT>
void OneUnknownSolver<InputT, OutputT>::init(InputT lo, InputT hi) {
    low = lo;
    high = hi;
    current = low;
    setParam(current);
}

template<typename InputT, typename OutputT>
bool OneUnknownSolver<InputT, OutputT>::stepFrame() {
    if (runSim()) { // if stop condition is reached
        OutputT measured = extract();
        OutputT error    = measured - target;
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
        return false;
    } else {
        return false;
    }
}

template class OneUnknownSolver<float, float>;