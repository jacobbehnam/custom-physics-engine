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

double linearTemperatureFactor(double coeffPerK, double tempK, double referenceTempK) {
    if (!std::isfinite(coeffPerK) || !std::isfinite(tempK) || !std::isfinite(referenceTempK)) return 1.0;
    return std::max(0.0, 1.0 + coeffPerK * (tempK - referenceTempK));
}

double effectiveSpecificHeat(const ThermalProperties& props, double tempK) {
    return static_cast<double>(std::max(props.specificHeat, 0.0f))
        * linearTemperatureFactor(props.specificHeatTempCoeff, tempK, props.referenceTempK);
}

double effectiveConductivity(const ThermalProperties& props, double tempK) {
    return static_cast<double>(std::max(props.conductivity, 0.0f))
        * linearTemperatureFactor(props.conductivityTempCoeff, tempK, props.referenceTempK);
}

double effectiveEmissivity(const ThermalProperties& props, double tempK) {
    const double value = static_cast<double>(std::clamp(props.emissivity, 0.0f, 1.0f))
        * linearTemperatureFactor(props.emissivityTempCoeff, tempK, props.referenceTempK);
    return std::clamp(value, 0.0, 1.0);
}

double effectiveAbsorptivity(const ThermalProperties& props, double tempK) {
    const double value = static_cast<double>(std::clamp(props.absorptivity, 0.0f, 1.0f))
        * linearTemperatureFactor(props.absorptivityTempCoeff, tempK, props.referenceTempK);
    return std::clamp(value, 0.0, 1.0);
}

double effectiveDensity(const ThermalProperties& props, double tempK) {
    const double density = static_cast<double>(std::max(props.density, 0.0f));
    if (density <= 0.0) return 0.0;

    const double expansion = 1.0 + 3.0 * static_cast<double>(props.linearExpansionCoeff) * (tempK - props.referenceTempK);
    return expansion > 0.0 ? density / expansion : density;
}

double activeThermalMass(double massKg, const ThermalProperties& props) {
    if (massKg <= 0.0) return 0.0;
    const double activeMassFraction = std::clamp(static_cast<double>(props.thermalMassFraction), 0.0, 1.0);
    return massKg * activeMassFraction;
}

double heatCapacity(double massKg, const ThermalProperties& props) {
    const double specificHeat = effectiveSpecificHeat(props, props.tempK);
    if (specificHeat <= 0.0) return 0.0;
    return activeThermalMass(massKg, props) * specificHeat;
}

double thermalDiffusivity(const ThermalProperties& props, double tempK) {
    const double density = effectiveDensity(props, tempK);
    const double specificHeat = effectiveSpecificHeat(props, tempK);
    if (density <= 0.0 || specificHeat <= 0.0) return 0.0;
    return effectiveConductivity(props, tempK) / (density * specificHeat);
}

double biotNumber(const ThermalProperties& props, double characteristicLengthM) {
    const double conductivity = effectiveConductivity(props, props.tempK);
    if (characteristicLengthM <= 0.0 || conductivity <= 0.0) return 0.0;
    return static_cast<double>(props.heatTransferCoeff) * characteristicLengthM / conductivity;
}

double fourierNumber(const ThermalProperties& props, double characteristicLengthM, double elapsedSeconds) {
    if (characteristicLengthM <= 0.0 || elapsedSeconds <= 0.0) return 0.0;
    return thermalDiffusivity(props, props.tempK) * elapsedSeconds / (characteristicLengthM * characteristicLengthM);
}

double carnotEfficiency(double hotTempK, double coldTempK) {
    hotTempK = clampTemperature(hotTempK);
    coldTempK = clampTemperature(coldTempK);
    if (hotTempK <= 0.0 || coldTempK >= hotTempK) return 0.0;
    return 1.0 - coldTempK / hotTempK;
}

double specificEntropyChange(double specificHeatJPerKgK, double fromTempK, double toTempK) {
    fromTempK = clampTemperature(fromTempK);
    toTempK = clampTemperature(toTempK);
    if (specificHeatJPerKgK <= 0.0 || fromTempK <= 0.0 || toTempK <= 0.0) return 0.0;
    return specificHeatJPerKgK * std::log(toTempK / fromTempK);
}

double convectionHeatRate(const ThermalProperties& props, double areaM2, double ambientTempK) {
    if (areaM2 <= 0.0 || props.heatTransferCoeff <= 0.0f) return 0.0;
    return static_cast<double>(props.heatTransferCoeff) * areaM2 * (ambientTempK - props.tempK);
}

double ambientRadiationHeatRate(const ThermalProperties& props, double areaM2, double ambientTempK) {
    const double tObj = clampTemperature(props.tempK);
    const double tAmb = clampTemperature(ambientTempK);
    const double emissivity = effectiveEmissivity(props, tObj);
    if (areaM2 <= 0.0 || emissivity <= 0.0) return 0.0;
    return emissivity * Constants::STEFAN_BOLTZMANN * areaM2 * (fourthPower(tAmb) - fourthPower(tObj));
}

double externalHeatFluxRate(const ThermalProperties& props, double areaM2) {
    if (areaM2 <= 0.0 || !std::isfinite(props.externalHeatFlux)) return 0.0;
    return props.externalHeatFlux * areaM2;
}

double conductiveHeatRate(double conductivityA, double conductivityB, double areaM2, double distanceM, double tempAK, double tempBK) {
    if (areaM2 <= 0.0 || distanceM <= 0.0 || conductivityA <= 0.0 || conductivityB <= 0.0) return 0.0;
    const double kEff = 2.0 * conductivityA * conductivityB / (conductivityA + conductivityB);
    return kEff * areaM2 * (tempBK - tempAK) / std::max(distanceM, kMinConductionDistance);
}

void applyThermalEnergy(ThermalProperties& props, double massKg, double energyJ) {
    const double capacity = heatCapacity(massKg, props);
    if (capacity <= 0.0 || energyJ == 0.0 || !std::isfinite(energyJ)) return;

    const double activeMass = activeThermalMass(massKg, props);
    const double fusionEnergy = activeMass * static_cast<double>(std::max(props.latentHeatFusion, 0.0f));
    const double vaporizationEnergy = activeMass * static_cast<double>(std::max(props.latentHeatVaporization, 0.0f));
    const double meltingPoint = static_cast<double>(std::max(props.meltingPoint, 0.0f));
    const double boilingPoint = static_cast<double>(std::max(props.boilingPoint, 0.0f));

    auto applySensibleHeatToward = [&](double targetTemp, double& energy) {
        const double initialTemp = props.tempK;
        const double specificHeat = effectiveSpecificHeat(props, initialTemp);
        const double required = (targetTemp - props.tempK) * capacity;
        if ((energy > 0.0 && required <= 0.0) || (energy < 0.0 && required >= 0.0)) return false;
        if (std::abs(energy) < std::abs(required)) {
            props.tempK = clampTemperature(props.tempK + energy / capacity);
            props.entropyJPerK += activeMass * specificEntropyChange(specificHeat, initialTemp, props.tempK);
            energy = 0.0;
            return true;
        }
        props.tempK = clampTemperature(targetTemp);
        props.entropyJPerK += activeMass * specificEntropyChange(specificHeat, initialTemp, props.tempK);
        energy -= required;
        return false;
    };

    auto applyLatentHeat = [&](double latentCapacity, float& progress, double& energy) {
        if (latentCapacity <= 0.0 || energy == 0.0) return;
        const double phaseTemp = std::max(props.tempK, 1.0e-9);

        if (energy > 0.0) {
            const double required = (1.0 - static_cast<double>(progress)) * latentCapacity;
            if (energy < required) {
                progress = static_cast<float>(std::clamp(static_cast<double>(progress) + energy / latentCapacity, 0.0, 1.0));
                props.entropyJPerK += energy / phaseTemp;
                energy = 0.0;
            } else {
                progress = 1.0f;
                props.entropyJPerK += required / phaseTemp;
                energy -= required;
            }
        } else {
            const double released = static_cast<double>(progress) * latentCapacity;
            const double cooling = -energy;
            if (cooling < released) {
                progress = static_cast<float>(std::clamp(static_cast<double>(progress) - cooling / latentCapacity, 0.0, 1.0));
                props.entropyJPerK += energy / phaseTemp;
                energy = 0.0;
            } else {
                progress = 0.0f;
                props.entropyJPerK -= released / phaseTemp;
                energy += released;
            }
        }
    };

    if (energyJ > 0.0) {
        if (meltingPoint > 0.0 && props.tempK < meltingPoint && applySensibleHeatToward(meltingPoint, energyJ)) return;
        if (meltingPoint > 0.0 && props.tempK <= meltingPoint && props.fusionProgress < 1.0f) {
            props.tempK = meltingPoint;
            applyLatentHeat(fusionEnergy, props.fusionProgress, energyJ);
            if (energyJ == 0.0) return;
        }

        if (boilingPoint > 0.0 && props.tempK < boilingPoint && applySensibleHeatToward(boilingPoint, energyJ)) return;
        if (boilingPoint > 0.0 && props.tempK <= boilingPoint && props.vaporizationProgress < 1.0f) {
            props.tempK = boilingPoint;
            applyLatentHeat(vaporizationEnergy, props.vaporizationProgress, energyJ);
            if (energyJ == 0.0) return;
        }

        const double initialTemp = props.tempK;
        props.tempK = clampTemperature(props.tempK + energyJ / capacity);
        props.entropyJPerK += activeMass * specificEntropyChange(effectiveSpecificHeat(props, initialTemp), initialTemp, props.tempK);
        return;
    }

    if (boilingPoint > 0.0 && props.tempK > boilingPoint && applySensibleHeatToward(boilingPoint, energyJ)) return;
    if (boilingPoint > 0.0 && props.tempK >= boilingPoint && props.vaporizationProgress > 0.0f) {
        props.tempK = boilingPoint;
        applyLatentHeat(vaporizationEnergy, props.vaporizationProgress, energyJ);
        if (energyJ == 0.0) return;
    }

    if (meltingPoint > 0.0 && props.tempK > meltingPoint && applySensibleHeatToward(meltingPoint, energyJ)) return;
    if (meltingPoint > 0.0 && props.tempK >= meltingPoint && props.fusionProgress > 0.0f) {
        props.tempK = meltingPoint;
        applyLatentHeat(fusionEnergy, props.fusionProgress, energyJ);
        if (energyJ == 0.0) return;
    }

    const double initialTemp = props.tempK;
    props.tempK = clampTemperature(props.tempK + energyJ / capacity);
    props.entropyJPerK += activeMass * specificEntropyChange(effectiveSpecificHeat(props, initialTemp), initialTemp, props.tempK);
}

void applyConductiveExchange(ThermalProperties& a, double massA, ThermalProperties& b, double massB, double areaM2, double distanceM, double dt) {
    if (dt <= 0.0) return;
    const double capA = heatCapacity(massA, a);
    const double capB = heatCapacity(massB, b);
    if (capA <= 0.0 || capB <= 0.0) return;

    const double rateToA = conductiveHeatRate(effectiveConductivity(a, a.tempK), effectiveConductivity(b, b.tempK), areaM2, distanceM, a.tempK, b.tempK);
    if (!std::isfinite(rateToA) || rateToA == 0.0) return;

    const double equilibriumEnergy = (b.tempK - a.tempK) * capA * capB / (capA + capB);
    double energyToA = rateToA * dt;
    if (std::abs(energyToA) > std::abs(equilibriumEnergy)) {
        energyToA = equilibriumEnergy;
    }

    applyThermalEnergy(a, massA, energyToA);
    applyThermalEnergy(b, massB, -energyToA);
}

}
