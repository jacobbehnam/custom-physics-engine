@page testing Testing and Validation

@brief Numerical correctness and validation strategy for the physics engine.

---

## Numerical Integration Method

The physics engine uses Velocity Verlet integration due to its stability and favorable energy conservation properties.

For general motion, the method has:
- Local truncation error of
    - \f$ O(\Delta t^4) \f$ for position
    - \f$ O(\Delta t^3) \f$ for velocity
- Accumulated global error over a fixed simulation time that scales as  
  \f$ O(\Delta t^2) \f$ for both position and velocity

---

## Constant-Force Special Case

The current simulation supports only constant forces (e.g., gravity and constant thrust).

In this case, acceleration satisfies:
\f[
\frac{d\ddot{\vec{r}}}{dt} = 0
\f]

Velocity Verlet integrates constant acceleration exactly, meaning the theoretical truncation error is zero. As a result, any deviation from the analytical solution arises solely from floating-point rounding error.

---

## Floating-Point Error Model

For constant-force test cases, the expected bound on position error is:

\f[
\lVert \vec{r}_{\text{error}} \rVert \le
C \cdot N \cdot \epsilon \cdot \max\!\left(\lVert \vec{r}(t) \rVert\right)
\f]

where:

- \f$ N \f$ is the number of integration steps  
  \f[
  N = \frac{1}{\Delta t}
  \f]

- \f$ \epsilon \f$ is the machine epsilon for single-precision floating point  
  \f[
  \epsilon \approx 1.19 \times 10^{-7}
  \f]

- \f$ \max(\lVert \vec{r}(t) \rVert) \f$ is the maximum magnitude of the position vector reached during the simulation

- \f$ C \f$ is a small conservative constant accounting for the number of floating-point operations per integration step

---

## Testing Strategy

Unit tests validate the physics core by:

- Comparing numerical results against closed-form analytical solutions
- Scaling tolerances according to the floating-point error bound above
- Verifying correctness across:
    - Zero acceleration
    - Constant velocity
    - Constant acceleration (free fall)
    - Multi-axis force composition
    - Mass scaling under identical forces

This approach ensures that test failures indicate true logical or numerical errors, rather than expected floating-point noise.
