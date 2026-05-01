#include "physics/utils/ThermalUtils.h"

#include <algorithm>
#include <cmath>
#include "physics/Constants.h"

namespace Physics::Thermal {

double fourthPower(double value) {
    const double square = value * value;
    return square * square;
}

double clampTemperature(double tempK) {
    if (!std::isfinite(tempK)) return kMinTemperatureK;
    return std::clamp(tempK, kMinTemperatureK, kMaxTemperatureK);
}

double heatCapacity(double massKg, const ThermalProperties& props) {
    if (massKg <= 0.0 || props.specificHeat <= 0.0f) return 0.0;
    return massKg * static_cast<double>(props.specificHeat);
}

double convectionHeatRate(const ThermalProperties& props, double areaM2, double ambientTempK) {
    if (areaM2 <= 0.0 || props.heatTransferCoeff <= 0.0f) return 0.0;
    return static_cast<double>(props.heatTransferCoeff) * areaM2 * (ambientTempK - props.tempK);
}

double ambientRadiationHeatRate(const ThermalProperties& props, double areaM2, double ambientTempK) {
    if (areaM2 <= 0.0 || props.emissivity <= 0.0f) return 0.0;
    const double tObj = clampTemperature(props.tempK);
    const double tAmb = clampTemperature(ambientTempK);
    return static_cast<double>(props.emissivity) * Constants::STEFAN_BOLTZMANN * areaM2 * (fourthPower(tAmb) - fourthPower(tObj));
}

double conductiveHeatRate(double conductivityA, double conductivityB, double areaM2, double distanceM, double tempAK, double tempBK) {
    if (areaM2 <= 0.0 || distanceM <= 0.0 || conductivityA <= 0.0 || conductivityB <= 0.0) return 0.0;
    const double kEff = 2.0 * conductivityA * conductivityB / (conductivityA + conductivityB);
    return kEff * areaM2 * (tempBK - tempAK) / std::max(distanceM, kMinConductionDistance);
}

void applyConductiveExchange(ThermalProperties& a, double massA, ThermalProperties& b, double massB, double areaM2, double distanceM, double dt) {
    if (dt <= 0.0) return;
    const double capA = heatCapacity(massA, a);
    const double capB = heatCapacity(massB, b);
    if (capA <= 0.0 || capB <= 0.0) return;

    const double rateToA = conductiveHeatRate(a.conductivity, b.conductivity, areaM2, distanceM, a.tempK, b.tempK);
    if (!std::isfinite(rateToA) || rateToA == 0.0) return;

    const double equilibriumEnergy = (b.tempK - a.tempK) * capA * capB / (capA + capB);
    double energyToA = rateToA * dt;
    if (std::abs(energyToA) > std::abs(equilibriumEnergy)) {
        energyToA = equilibriumEnergy;
    }

    a.tempK = clampTemperature(a.tempK + energyToA / capA);
    b.tempK = clampTemperature(b.tempK - energyToA / capB);
}

}
