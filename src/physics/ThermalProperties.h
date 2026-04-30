#pragma once

struct ThermalProperties {
    double tempK            = 293.15;  // 20C, standard room temperature
    float specificHeat      = 450.0f;  // J/(kg·K)  - iron
    float emissivity        = 0.70f;   // 0–1       - oxidized metal
    float heatTransferCoeff = 10.0f;   // W/(m²·K)  - still air natural convection
    float conductivity      = 79.0f;   // W/(m·K)   - iron
    float density           = 7874.0f; // kg/m³     - iron
    float meltingPoint      = 1811.0f; // Kelvin    - iron, 0 if N/A
};
