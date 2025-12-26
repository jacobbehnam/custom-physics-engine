#include "InterceptSolver.h"
#include <iostream>
#include <utility>

InterceptSolver::InterceptSolver(MonitorFunction monitor, TimeoutCondition timeout) :
      monitorFunc(std::move(monitor)), timeoutFunc(std::move(timeout)) {}

bool InterceptSolver::stepFrame() {
      // Check limits
      if (timeoutFunc && timeoutFunc()) {
            std::cout << "[InterceptSolver] Timeout/Safety limit reached." << std::endl;
            return true; // Stop (Safety exit)
      }

      // Check the physics condition
      if (monitorFunc) {
            float val = monitorFunc();
            if (val <= 0.0f) {
                  return true; // Stop (Success exit)
            }
      }

      return false; // Keep running
}
