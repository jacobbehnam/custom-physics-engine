#include "VectorRootSolver.h"
#include <iostream>

template<typename InputT, typename OutputT>
VectorRootSolver<InputT, OutputT>::VectorRootSolver(InitialGuessSetter initialGuessSetter, StopCondition stopCondition, ResultExtractor extractResult, const OutputT &tgt, double tol, int maxIter, double jacobianStep, double damping)
    : setGuess(std::move(initialGuessSetter)), stopCondition(std::move(stopCondition)), extract(std::move(extractResult)), target(tgt), tolerance(tol), current(static_cast<InputT>(0)), h(jacobianStep), alpha(damping) {
    current = glm::vec3(0.0f);
    fPerturbed.fill(glm::vec3(0.0f));
    state = NewtonState::WaitingForBase;
}

template<typename InputT, typename OutputT>
bool VectorRootSolver<InputT, OutputT>::stepFrame() {
    switch (state) {

        case NewtonState::WaitingForBase:
            if (!stopCondition()) return false;

            baseOutput = extract();

            currentPerturbation = 0;
            state = NewtonState::PerturbComponent;
            return false;

        case NewtonState::PerturbComponent:
            xPerturbed = current;
            xPerturbed[currentPerturbation] += h;

            setGuess(xPerturbed); // resets simulation with perturbed guess
            state = NewtonState::WaitingForPerturbed;
            return false;

        case NewtonState::WaitingForPerturbed:
            if (!stopCondition()) return false;

            fPerturbed[currentPerturbation] = extract();

            currentPerturbation++;
            if (currentPerturbation < 3)
                state = NewtonState::PerturbComponent;
            else
                state = NewtonState::ComputeJacobian;

            return false;

        case NewtonState::ComputeJacobian: {
            glm::mat3 J(0.0f);
            for (int j = 0; j < 3; ++j)
                for (int i = 0; i < 3; ++i) {
                    J[i][j] = (fPerturbed[j][i] - baseOutput[i]) / h;
                }

            glm::vec3 error = baseOutput - target;

            float detJ = glm::determinant(J);

            if (std::abs(detJ) < 1e-8f) {
                return false; // skip step to prevent NaNs
            }

            glm::vec3 dx = -alpha * glm::inverse(J) * error;
            current += dx;
            setGuess(current);

            // reset for next Newton iteration
            state = NewtonState::WaitingForBase;

            return glm::length(error) < tolerance;
        }

        }

        return false;
}



template class VectorRootSolver<glm::vec3, glm::vec3>;