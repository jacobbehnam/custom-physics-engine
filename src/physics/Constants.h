#pragma once

namespace Constants {
    constexpr double G_SCALED           = 6.6743;       // Scaled gravitational constant for better numerical stability in simulations
    constexpr double G                  = 6.67430e-11;  // Gravitational constant
    constexpr double STEFAN_BOLTZMANN   = 5.670374419e-8; // W/(m^2*K^4)
    constexpr float THETA_SQ            = 0.5f;         // Barnes-Hut threshold (squared)
    constexpr float SOFTENING_SQ        = 0.01f;        // Softening factor (squared)
    constexpr float STANDARD_GRAVITY    = 9.81f;        // EARTH gravity
}
