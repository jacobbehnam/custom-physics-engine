/**
 * @file ISolver.h
 * @brief Abstract interface for numerical solvers
 * @ingroup solver
 *
 * Defines the common interface for all numerical solvers in the system.
 * Solvers are designed to work asynchronously with the physics simulation,
 * advancing one step at a time and signaling when they've converged or failed.
 */

#pragma once

/**
 * @brief Abstract base class for all numerical solvers
 * @ingroup solver
 *
 * ISolver provides a minimal interface for solvers that operate on physics
 * simulations or other iterative processes. The solver is called repeatedly
 * by the physics system until it signals completion.
 *
 * ## Design Philosophy
 *
 * Solvers are intentionally decoupled from physics-specific code. They operate
 * on generic callables (std::function) rather than physics objects directly.
 * This allows the same solver infrastructure to be reused for different problem
 * domains.
 *
 * ## Usage Pattern
 *
 * 1. Construct solver with problem-specific functions
 * 2. Physics system calls stepFrame() each simulation step
 * 3. Solver returns false until convergence/completion
 * 4. Solver returns true when done (success or failure)
 *
 * ## Implementations
 *
 * - InterceptSolver: Forward problems (monitors until condition met)
 * - VectorRootSolver: Inverse problems (Newton's method for vector unknowns)
 *
 * @see InterceptSolver, VectorRootSolver
 * @see ProblemRouter for automatic solver selection
 *
 * Example usage:
 * @code
 * // Create a solver
 * auto solver = std::make_unique<InterceptSolver>(
 *     []() { return currentHeight - targetHeight; },  // monitor function
 *     []() { return simTime > 60.0f; }                // timeout
 * );
 *
 * // In physics loop
 * while (!solver->stepFrame()) {
 *     // Continue simulation
 *     physicsSystem->step(dt);
 * }
 * // Solver has finished
 * @endcode
 */
class ISolver {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     */
    virtual ~ISolver() = default;

    /**
     * @brief Advances the solver by one step
     *
     * This method is called by the physics system after each simulation step.
     * The solver examines the current state and decides whether to continue
     * or declare completion.
     *
     * ## Typical Implementation Pattern
     *
     * Forward solvers (InterceptSolver):
     * - Check if stop condition is met
     * - Return true if condition satisfied or timeout
     *
     * Inverse solvers (VectorRootSolver):
     * - Extract current output from simulation
     * - Compute error vs target
     * - Adjust input guess
     * - Reset simulation with new guess
     * - Return true if converged
     *
     * @return true if solver has finished (converged, intercepted, or failed)
     * @return false if solver needs to continue
     *
     * @note This method should NOT advance the physics simulation itself.
     *       The physics system handles that externally.
     *
     * @thread Called from physics thread; must be thread-safe if solver
     *         accesses shared state
     */
    virtual bool stepFrame() = 0;
};