# Physics Simulation & Visualization Tool

A C++ physics simulation and visualization tool for modeling and solving classical mechanics problems through real-time 3D interaction and numerical methods.

## Overview
This project is a custom-built physics simulation and visualization tool designed to model classical mechanics problems in an interactive 3D environment. Users can construct physical scenarios, manipulate objects directly in the scene, and either simulate their behavior over time or numerically solve for unknown quantities.

While similar tools and engines already exist, this project was built from the ground up as an exercise in understanding computational physics, numerical methods, and modern OpenGL rendering, with a strong emphasis on clean architecture and maintainability. It also explores how visual tooling can help bridge the gap between abstract equations and physical intuition.

## Example Scenario
![Physics Demo](src/assets/demo.gif)
A set of keys is dropped from rest from a height of 20 m, while a ball is simultaneously thrown upward from the ground with an initial velocity of 15 m/s. The system numerically determines the height at which the two objects pass each other.

## Core Features
### Physics Simulation
- 3D physics simulation
- Velocity Verlet integration for stable, energy-conserving motion
- Support for point masses and rigid bodies
- Gravity and arbitrary constant forces
- Collision detection and resolution for
  - Point mass $\leftrightarrow$ point mass
  - Point mass $\leftrightarrow$ rigid body

### Numerical Solvers
- Solvers are templated and decoupled from physics data structures, operating purely on generic callables, allowing reuse outside of physics
- Physics problems are routed to solvers via a dedicated problem-routing layer
- Intermediate solver iterations are exposed to the user, allowing visualization of convergence rather than only final results

### Interactive 3D Visualization
- Real-time OpenGL renderer with free-fly camera controls
- Object selection and manipulation directly in the scene
- Transform gizmos for translation, rotation, and scaling
- Compute shaders (SSBOs) used for GPU-based ray intersection tests

### Editor-Style UI
- Qt-based application interface
- Scene hierarchy and inspector panels
- Live modification of object properties
- Controls for running, pausing, and solving simulations
- Snapshot system that records full simulation state at each timestep for debugging and analysis

### Multithreaded Simulation
- Physics simulation runs on a dedicated thread, decoupled from rendering and UI
- Ensures responsive interaction while simulations or numerical solvers are running

## Architecture Overview
This project is organized into two major layers and executes across multiple threads to maintain responsiveness during simulation and problem-solving. The physics simulation runs on a dedicated thread, while rendering and UI logic execute independently on the main thread:
### PhysicsCore
A standalone static library containing:
- Physics bodies and simulation logic
- Collision handling
- Numerical solvers
- Math and utility code

This layer is designed to be independent of rendering and UI concerns

### Testing and Validation
The physics core is designed to be testable in isolation from rendering and UI. Unit tests focus on numerical correctness and regression protection as the system evolves. More information about testing in [Testing and Validation](@ref testing).

### Application layer
- OpenGL rendering and scene management
- Qt-based user interface
- Editor tools and interaction logic
- Visualization of physics state and solver progress

A key architectural goal is separation of concerns. Numerical solvers are intentionally decoupled from physics-specific code, allowing the same solver infrastructure to be reused for different problem domains.

## Project Status
This is an ongoing independent project and work in progress. The codebase prioritizes clarity, modularity, and extensibility, even when features are incomplete. The current implementation represents a stable foundation for future experimentation and refinement.

## Author
Jacob Behnam