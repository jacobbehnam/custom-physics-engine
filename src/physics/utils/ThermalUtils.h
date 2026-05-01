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
double heatCapacity(double massKg, const ThermalProperties& props);
double convectionHeatRate(const ThermalProperties& props, double areaM2, double ambientTempK);
double ambientRadiationHeatRate(const ThermalProperties& props, double areaM2, double ambientTempK);
double conductiveHeatRate(double conductivityA, double conductivityB, double areaM2, double distanceM, double tempAK, double tempBK);
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

        props.tempK = clampTemperature(props.tempK + (rate * subDt) / capacity);
        remaining -= subDt;
    }
}

}
