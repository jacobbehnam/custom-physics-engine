#pragma once

#include <algorithm>
#include <cmath>
#include "physics/ThermalProperties.h"

namespace Physics::Thermal {

constexpr double kMinTemperatureK = 0.0;
constexpr double kMaxTemperatureK = 1.0e8;
constexpr double kMinConductionDistance = 1.0e-9;
constexpr double kMaxThermalStepSeconds = 60.0;
constexpr double kMaxTemperatureStepK = 25.0;
constexpr double kMaxTemperatureStepFraction = 0.02;

double fourthPower(double value);
double clampTemperature(double tempK);
double linearTemperatureFactor(double coeffPerK, double tempK, double referenceTempK);
double effectiveSpecificHeat(const ThermalProperties& props, double tempK);
double effectiveConductivity(const ThermalProperties& props, double tempK);
double effectiveEmissivity(const ThermalProperties& props, double tempK);
double effectiveAbsorptivity(const ThermalProperties& props, double tempK);
double effectiveDensity(const ThermalProperties& props, double tempK);
double activeThermalMass(double massKg, const ThermalProperties& props);
double heatCapacity(double massKg, const ThermalProperties& props);
double thermalDiffusivity(const ThermalProperties& props, double tempK);
double biotNumber(const ThermalProperties& props, double characteristicLengthM);
double fourierNumber(const ThermalProperties& props, double characteristicLengthM, double elapsedSeconds);
double carnotEfficiency(double hotTempK, double coldTempK);
double specificEntropyChange(double specificHeatJPerKgK, double fromTempK, double toTempK);
double convectionHeatRate(const ThermalProperties& props, double areaM2, double ambientTempK);
double ambientRadiationHeatRate(const ThermalProperties& props, double areaM2, double ambientTempK);
double externalHeatFluxRate(const ThermalProperties& props, double areaM2);
double conductiveHeatRate(double conductivityA, double conductivityB, double areaM2, double distanceM, double tempAK, double tempBK);
void applyThermalEnergy(ThermalProperties& props, double massKg, double energyJ);
void applyConductiveExchange(ThermalProperties& a, double massA, ThermalProperties& b, double massB, double areaM2, double distanceM, double dt);

template <typename HeatRateFn>
void integrateTemperature(ThermalProperties& props, double massKg, double dt, HeatRateFn&& heatRateAtTemp) {
    if (dt <= 0.0) return;
    const double capacity = heatCapacity(massKg, props);
    if (capacity <= 0.0) return;

    double remaining = dt;
    int guard = 0;
    while (remaining > 0.0 && guard++ < 4096) {
        const double rate = heatRateAtTemp(props.tempK);
        if (!std::isfinite(rate) || rate == 0.0) break;

        const double maxDelta = std::max(kMaxTemperatureStepK, std::abs(props.tempK) * kMaxTemperatureStepFraction);
        const double stepByTemp = maxDelta * capacity / std::abs(rate);
        const double subDt = std::min({remaining, kMaxThermalStepSeconds, stepByTemp});
        if (subDt <= 0.0 || !std::isfinite(subDt)) break;

        applyThermalEnergy(props, massKg, rate * subDt);
        remaining -= subDt;
    }
}

}
