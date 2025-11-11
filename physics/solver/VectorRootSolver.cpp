#include "VectorRootSolver.h"
#include <iostream>

template<typename InputT, typename OutputT>
VectorRootSolver<InputT, OutputT>::VectorRootSolver(InitialGuessSetter initialGuessSetter, StopCondition stopCondition, ResultExtractor extractResult, const OutputT &tgt, double tol, int maxIter, double jacobianStep, double damping)
    : setGuess(std::move(initialGuessSetter)), stopCondition(std::move(stopCondition)), extract(std::move(extractResult)), target(tgt), tolerance(tol), current(static_cast<InputT>(0)), h(jacobianStep), alpha(damping) {
    current = glm::vec3(0.0f); // Our initial guess is all zeroes
    fPerturbed.fill(glm::vec3(0.0f));
    state = SolverState::WaitingForBase;
}

template<typename InputT, typename OutputT>
bool VectorRootSolver<InputT, OutputT>::stepFrame() {
    switch (state) {
        case SolverState::WaitingForBase:
            // Wait until the simulation has run for the current guess.
            // This ensures we have the base output for the current input.
            if (!stopCondition()) return false;

            baseOutput = extract();

            currentPerturbation = 0;
            state = SolverState::PerturbComponent;
            return false;

        case SolverState::PerturbComponent:
            // Create a perturbed input by slightly incrementing the current component
            xPerturbed = current;
            xPerturbed[currentPerturbation] += h;

            setGuess(xPerturbed); // resets simulation with perturbed guess
            state = SolverState::WaitingForPerturbed; // Wait for the simulation to finish with this perturbed guess
            return false;

        case SolverState::WaitingForPerturbed:
            // Wait until the simulation has run for the perturbed input
            if (!stopCondition()) return false;

            fPerturbed[currentPerturbation] = extract();

            currentPerturbation++;
            // Move to the next component to perturb
            if (currentPerturbation < 3) // TODO: currently only works for F: R^3 -> R^3
                state = SolverState::PerturbComponent;
            else
                state = SolverState::ComputeJacobian;

            return false;

        case SolverState::ComputeJacobian: {
            glm::mat3 J(0.0f);
            for (int j = 0; j < 3; ++j)
                for (int i = 0; i < 3; ++i) {
                    // This is an approximation for the partial derivative for small h
                    J[i][j] = (fPerturbed[j][i] - baseOutput[i]) / h;
                }

            glm::vec3 error = baseOutput - target;

            float detJ = glm::determinant(J);
            glm::vec3 dx;

            if (std::abs(detJ) < 1e-8f) {
                // Jacobian nearly singular, apply a small nudge in direction of negative error
                constexpr float nudgeFactor = 0.01f; // tweak as needed
                dx = -nudgeFactor * error;
            } else {
                // Normal Newton step with damping
                dx = -alpha * glm::inverse(J) * error;
            }

            current += dx;
            setGuess(current);
            state = SolverState::WaitingForBase;

            // Return true if solution has converged (error below tolerance)
            return glm::length(error) < tolerance;
        }
        }
        return false;
}

template class VectorRootSolver<glm::vec3, glm::vec3>;