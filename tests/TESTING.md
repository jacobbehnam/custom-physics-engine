The physics engine uses Velocity Verlet for integration due to its stability and energy conservation accuracy. While the local truncation error is $O(\Delta t^4)$ for position and $O(\Delta t^3)$ for velocity, the accumulated global error over a fixed simulation time scales as $O(\Delta t^2)$ for both quantities.

Currently, the simulation only supports constant forces (gravity, constant thrust). Since Velocity Verlet integrates constant acceleration exactly ($\frac{\mathrm{d}\ddot{\vec{r}}}{\mathrm{d}t} = 0$), the method truncation error is theoretically zero. For these test cases, the expected error in position is solely due to floating point rounding error, bounded by:
$$
\|\vec{r}_{\text{error}}\| \le C \cdot  N\cdot  \epsilon \cdot \text{max}(\|\vec{r}(t)\|)
$$
where $N$ is the number of steps ($\frac{1}{\Delta t}$), $\epsilon$ is the machine epsilon for a float ($1.19 \times 10^{-7}$), $\text{max}(\vec{r}(t))$ is the maximum magnitude of the position vector reached during the simulation, and $C$ is a small conservative constant accounting for floating-point operations per integration step.