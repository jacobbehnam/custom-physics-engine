#pragma once

struct ThermalProperties {
    double tempK            = 293.15;  // 20C, standard room temperature
    double internalHeatPower = 0.0;    // W          - generated heat inside the body
    double externalHeatFlux = 0.0;     // W/m2       - net absorbed heat flux on the surface
    float specificHeat      = 450.0f;  // J/(kg·K)  - iron
    float thermalMassFraction = 1.0f;  // 0–1       - fraction of body mass thermally active
    float emissivity        = 0.70f;   // 0–1       - oxidized metal
    float absorptivity      = 0.70f;   // 0–1       - absorbed incident radiation
    float heatTransferCoeff = 10.0f;   // W/(m²·K)  - still air natural convection
    float conductivity      = 79.0f;   // W/(m·K)   - iron
    float density           = 7874.0f; // kg/m³     - iron
    float meltingPoint      = 1811.0f; // Kelvin    - iron, 0 if N/A
    float latentHeatFusion  = 0.0f;    // J/kg       - energy absorbed while melting
    float fusionProgress    = 0.0f;    // 0–1        - 0 solid, 1 liquid
    float boilingPoint      = 0.0f;    // Kelvin     - 0 if N/A
    float latentHeatVaporization = 0.0f; // J/kg     - energy absorbed while boiling
    float vaporizationProgress = 0.0f; // 0–1        - 0 condensed, 1 vapor
};
