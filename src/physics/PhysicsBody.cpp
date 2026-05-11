#include "physics/PhysicsBody.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <glm/common.hpp>
#include "physics/utils/ThermalUtils.h"

namespace {

constexpr float kVisibleIncandescenceStartK       = 800.0f;
constexpr float kVisibleEmitterReferenceK         = 5800.0f;
constexpr float kVisibleEmitterReferenceRadiance  = 65000.0f;

} // namespace

bool Physics::PhysicsBody::isUnknown(const std::string &key, BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return unknowns.find(key) != unknowns.end();
}

void Physics::PhysicsBody::setUnknown(const std::string &key, bool active, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    if (active) {
        unknowns.insert(key);
    } else {
        unknowns.erase(key);
    }
}

void Physics::PhysicsBody::setForce(const std::string &name, const glm::vec3 &force, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    // Recalculate net force
    auto it = forces.find(name);
    if (it != forces.end()) {
        // Its old value we remove it
        netForce -= it->second;
        it->second = force;
    } else {
        forces[name] = force;
    }
    // Just add new force
    netForce += force;
}

glm::vec3 Physics::PhysicsBody::getForce(const std::string &name, BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    auto it = forces.find(name);
    if (it == forces.end()) {
        return glm::vec3(0.0f); // Return zero vector if key doesn't exist
    }

    return forces.find(name)->second;
}

std::map<std::string, glm::vec3> Physics::PhysicsBody::getAllForces(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return forces;
}

glm::vec3 Physics::PhysicsBody::getNetForce(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return netForce;
}

glm::vec3 Physics::PhysicsBody::getPosition(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return position;
}

void Physics::PhysicsBody::setPosition(const glm::vec3 &pos, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    position = pos;
}

glm::vec3 Physics::PhysicsBody::getVelocity(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return velocity;
}

void Physics::PhysicsBody::setVelocity(const glm::vec3 &vel, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    velocity = vel;
}

double Physics::PhysicsBody::getMass(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return mass;
}

void Physics::PhysicsBody::setMass(double newMass, BodyLock lock) {
    if (newMass <= 0.0) {
        std::cerr << "Warning: Invalid mass " << newMass << ". Ignoring." << std::endl;
        return;
    }

    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);
    mass = newMass;
}

bool Physics::PhysicsBody::getIsStatic(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return isStatic;
}

void Physics::PhysicsBody::setIsStatic(bool flag, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    isStatic = flag;
}

glm::mat4 Physics::PhysicsBody::getWorldTransform(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return worldMatrix;
}

void Physics::PhysicsBody::setWorldTransform(const glm::mat4 &M, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    worldMatrix = M;
}

void Physics::PhysicsBody::clearAllFrames(BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    frames.clear();
}

void Physics::PhysicsBody::setTemperature(float kelvin, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);
    thermalProps.tempK = Physics::Thermal::clampTemperature(kelvin);
}

float Physics::PhysicsBody::getTemperature(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);
    return static_cast<float>(thermalProps.tempK);
}

glm::vec3 Physics::PhysicsBody::blackbodyRGB(float t) {
    if (t <= 0.0f) return glm::vec3(0.0f);
    float k = t / 100.0f;
    glm::vec3 col;

    col.r = k <= 66.0f
        ? 1.0f
        : glm::clamp(1.2929f * std::pow(k - 60.0f, -0.1332f), 0.0f, 1.0f);

    col.g = k <= 66.0f
        ? glm::clamp(0.3900f * std::log(k) - 0.6318f, 0.0f, 1.0f)
        : glm::clamp(1.1298f * std::pow(k - 60.0f, -0.0755f), 0.0f, 1.0f);

    col.b = k >= 66.0f ? 1.0f
          : k <= 19.0f ? 0.0f
          : glm::clamp(0.5432f * std::log(k - 10.0f) - 1.1963f, 0.0f, 1.0f);

    return col;
}

float Physics::PhysicsBody::temperatureToIntensity(float tempKelvin) {
    if (tempKelvin < kVisibleIncandescenceStartK) {
        return 0.0f;
    }

    const float visibleT = (tempKelvin - kVisibleIncandescenceStartK)
        / (kVisibleEmitterReferenceK - kVisibleIncandescenceStartK);
    return std::pow(std::max(visibleT, 0.0f), 4.0f) * kVisibleEmitterReferenceRadiance;
}

glm::vec3 Physics::PhysicsBody::getEmission(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);
    glm::vec3 emission = thermalProps.visibleLightColor * thermalProps.visibleLightPower;

    const float tempK = static_cast<float>(thermalProps.tempK);
    const float thermalEmissivity = static_cast<float>(Physics::Thermal::effectiveEmissivity(thermalProps, tempK));
    if (tempK > 0.0f && thermalEmissivity > 0.0f) {
        emission += blackbodyRGB(tempK) * temperatureToIntensity(tempK) * thermalEmissivity;
    }
    return emission;
}

void Physics::PhysicsBody::setThermalProperty(const ThermalProperties &newProps, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    thermalProps = newProps;
    thermalProps.tempK = Physics::Thermal::clampTemperature(thermalProps.tempK);
    if (!std::isfinite(thermalProps.internalHeatPower)) thermalProps.internalHeatPower = 0.0;
    if (!std::isfinite(thermalProps.externalHeatFlux)) thermalProps.externalHeatFlux = 0.0;
    if (!std::isfinite(thermalProps.entropyJPerK)) thermalProps.entropyJPerK = 0.0;
    thermalProps.referenceTempK = Physics::Thermal::clampTemperature(thermalProps.referenceTempK);
    thermalProps.specificHeat = std::max(thermalProps.specificHeat, 0.0f);
    if (!std::isfinite(thermalProps.specificHeatTempCoeff)) thermalProps.specificHeatTempCoeff = 0.0f;
    thermalProps.thermalMassFraction = std::clamp(thermalProps.thermalMassFraction, 0.0f, 1.0f);
    thermalProps.emissivity = std::clamp(thermalProps.emissivity, 0.0f, 1.0f);
    if (!std::isfinite(thermalProps.emissivityTempCoeff)) thermalProps.emissivityTempCoeff = 0.0f;
    if (!std::isfinite(thermalProps.visibleLightPower)) thermalProps.visibleLightPower = 0.0f;
    thermalProps.visibleLightPower = std::max(thermalProps.visibleLightPower, 0.0f);
    if (!std::isfinite(thermalProps.visibleLightColor.r)) thermalProps.visibleLightColor.r = 1.0f;
    if (!std::isfinite(thermalProps.visibleLightColor.g)) thermalProps.visibleLightColor.g = 1.0f;
    if (!std::isfinite(thermalProps.visibleLightColor.b)) thermalProps.visibleLightColor.b = 1.0f;
    thermalProps.visibleLightColor = glm::clamp(thermalProps.visibleLightColor, glm::vec3(0.0f), glm::vec3(1.0f));
    thermalProps.absorptivity = std::clamp(thermalProps.absorptivity, 0.0f, 1.0f);
    if (!std::isfinite(thermalProps.absorptivityTempCoeff)) thermalProps.absorptivityTempCoeff = 0.0f;
    thermalProps.heatTransferCoeff = std::max(thermalProps.heatTransferCoeff, 0.0f);
    thermalProps.conductivity = std::max(thermalProps.conductivity, 0.0f);
    if (!std::isfinite(thermalProps.conductivityTempCoeff)) thermalProps.conductivityTempCoeff = 0.0f;
    thermalProps.density = std::max(thermalProps.density, 0.0f);
    if (!std::isfinite(thermalProps.linearExpansionCoeff)) thermalProps.linearExpansionCoeff = 0.0f;
    thermalProps.meltingPoint = std::max(thermalProps.meltingPoint, 0.0f);
    thermalProps.latentHeatFusion = std::max(thermalProps.latentHeatFusion, 0.0f);
    thermalProps.fusionProgress = std::clamp(thermalProps.fusionProgress, 0.0f, 1.0f);
    thermalProps.boilingPoint = std::max(thermalProps.boilingPoint, 0.0f);
    thermalProps.latentHeatVaporization = std::max(thermalProps.latentHeatVaporization, 0.0f);
    thermalProps.vaporizationProgress = std::clamp(thermalProps.vaporizationProgress, 0.0f, 1.0f);
}

ThermalProperties Physics::PhysicsBody::getThermalProperties(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return thermalProps;
}
