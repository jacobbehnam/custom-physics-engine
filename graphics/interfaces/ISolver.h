#pragma once

/**
 * @brief Abstract interface for any physics solver step logic.
 */
class ISolver {
public:
    virtual ~ISolver() = default;

    /**
     * @brief Advances the solver logic by one step/frame.
     * @return true if the solver has finished (successfully converged, intercepted, or failed).
     * @return false if the solver needs to keep running.
     */
    virtual bool stepFrame() = 0;
};