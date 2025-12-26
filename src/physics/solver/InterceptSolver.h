#pragma once
#include "ISolver.h"
#include <functional>

/**
 * @brief A linear "monitor" solver that stops simulation when a condition is met.
 *
 * This solver is used for "Forward Problems" where the initial conditions are known,
 * and we simply need to run the simulation until a specific event occurs (e.g.,
 * reaching a peak, hitting a target, or reaching a specific time).
 *
 * Unlike VectorRootSolver, this does NOT iterate or reset the simulation repeatedly.
 * It runs a single trajectory and returns 'true' when the event is detected.
 */
class InterceptSolver : public ISolver {
public:
    // Returns > 0 to continue, <= 0 to stop.
    using MonitorFunction = std::function<float()>;

    // Returns true to abort (fail/timeout).
    using TimeoutCondition = std::function<bool()>;

    /**
     * @brief Construct a new Intercept Solver
     *
     * @param monitorFunc Function returning the metric to monitor. Solver stops when this drops <= 0.
     * @param timeoutFunc (Optional) Safety abort check.
     */
    explicit InterceptSolver(MonitorFunction monitorFunc, TimeoutCondition timeoutFunc = nullptr);

    /**
     * @brief Checks the current simulation state against the intercept conditions.
     *
     * @return true if the intercept event occurred (metric <= 0), success condition met, or timeout.
     * @return false otherwise.
     */
    bool stepFrame() override;

private:
    MonitorFunction monitorFunc;
    TimeoutCondition timeoutFunc;
};