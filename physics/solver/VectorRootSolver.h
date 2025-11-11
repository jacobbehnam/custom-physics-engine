#pragma once
#include <functional>
#include <glm/glm.hpp>

/**
 * @brief A vector-valued root solver using Newton's method.
 *
 * This class attempts to find the input vector `current` such that
 * the output of a function (extracted via `ResultExtractor`) matches
 * a given target vector. It works in n-dimensional space (TODO: currently specialized for 3D vectors),
 * using finite-difference approximations for the Jacobian.
 *
 * The solver is designed to work asynchronously with a simulation loop:
 * - It does not advance the simulation internally.
 * - External code calls `stepFrame()` repeatedly after the simulation has advanced.
 */
template<typename InputT, typename OutputT>
class VectorRootSolver {
public:
    using InitialGuessSetter = std::function<void(const InputT&)>; // Sets the value for the unknown parameter we are solving for
    using StopCondition = std::function<bool()>; // Runs the simulation up until a stop condition is reached
    using ResultExtractor = std::function<OutputT()>; // Gets a value to compare to the target value after an iteration

    /**
     * @brief Construct a new VectorRootSolver object
     *
     * @param initialGuessSetter Function to apply a new guess for the unknown input
     * @param stopCondition Function to check if the simulation has reached the evaluation point
     * @param extractResult Function to extract the output vector at the current guess
     * @param target The target output vector the solver tries to reach
     * @param tolerance Convergence tolerance for the error ||F(x) - target||
     * @param maxIterations Maximum number of Newton iterations before giving up
     * @param jacobianStep Step size for finite-difference Jacobian approximation
     * @param damping Damping factor (alpha) for Newton step
     */
    VectorRootSolver(InitialGuessSetter initialGuessSetter,
                     StopCondition stopCondition,
                     ResultExtractor extractResult,
                     const OutputT& target,
                     double tolerance = 1e-3,
                     int maxIterations = 30,
                     double jacobianStep = 0.01,
                     double damping = 1.0);

    /**
     * @brief Performs one iteration of the vector root-finding solver.
     *
     * This function attempts to improve the current guess for the unknown input
     * by performing one step of Newton's method in N dimensions. It uses the current
     * output (from `extract()`) and the target value to compute the error vector,
     * approximates the Jacobian, and computes the next guess for the input using:
     *
     * x_{n+1} = x_n - J(x_n)^{-1} * F(x_n)
     *
     * where x_n is the current guess and F(x_n) = extract() - target.
     *
     * The guess is applied via the `setGuess()` function.
     *
     * @note The actual simulation should be run externally. This function
     * only updates the solver state and the next guess based on the most recent output.
     *
     * @return true if the error norm ||F(x_n)|| is less than the solver's tolerance,
     * indicating convergence to a root; false otherwise.
     */
    bool stepFrame();

private:
    enum class SolverState {
        WaitingForBase, PerturbComponent, WaitingForPerturbed, ComputeJacobian
    };
    SolverState state;

    InitialGuessSetter setGuess;
    StopCondition stopCondition;
    ResultExtractor extract;

    InputT current;   // current guess for unknown(s)
    glm::vec3 target;
    glm::vec3 baseOutput;
    glm::vec3 xPerturbed;
    std::array<glm::vec3, 3> fPerturbed;

    int currentPerturbation = -1;
    float h;
    float tolerance;
    float alpha;
};