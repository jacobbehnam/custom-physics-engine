#include "VectorRootSolver.h"

template<typename InputT, typename OutputT>
VectorRootSolver<InputT, OutputT>::VectorRootSolver(InitialGuessSetter initialGuessSetter, StopCondition stopCondition, ResultExtractor extractResult, const OutputT &tgt, double tol, int maxIter)
    : setGuess(std::move(initialGuessSetter)), stopCondition(std::move(stopCondition)), extract(std::move(extractResult)), target(tgt), tolerance(tol), current(static_cast<InputT>(0)) {
    setGuess(current);
}

template<typename InputT, typename OutputT>
bool VectorRootSolver<InputT, OutputT>::stepFrame() {

}
