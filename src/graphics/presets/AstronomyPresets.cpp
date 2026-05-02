#include "graphics/presets/AstronomyPresets.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"
#include "physics/Constants.h"

namespace ScenePresets::Astronomy {

void createRealSolarSystem(SceneManager& sceneManager) {
    constexpr double metersPerKm = 1000.0;
    constexpr double metersPerAu = 149597870700.0;
    constexpr double defaultEpochJd = 2461161.5;
    constexpr double j2000Jd = 2451545.0;
    constexpr double julianCenturyDays = 36525.0;
    constexpr double pi = 3.14159265358979323846264338327950288;
    constexpr double degToRad = pi / 180.0;

    struct OrbitalElements {
        double semiMajorAxisAu;
        double semiMajorAxisAuPerCentury;
        double eccentricity;
        double eccentricityPerCentury;
        double inclinationDeg;
        double inclinationDegPerCentury;
        double meanLongitudeDeg;
        double meanLongitudeDegPerCentury;
        double longitudePerihelionDeg;
        double longitudePerihelionDegPerCentury;
        double longitudeAscendingNodeDeg;
        double longitudeAscendingNodeDegPerCentury;
    };

    struct OrbitalState {
        glm::vec3 position;
        glm::vec3 velocity;
    };

    struct ThermalSpec {
        double tempK;
        float density;
        float bondAlbedo;
        float specificHeat;
        float thermalMassFraction;
        float conductivity;
        float linearExpansionCoeff;
        float meltingPoint;
        float latentHeatFusion;
        float boilingPoint;
        float latentHeatVaporization;
    };

    struct BodySpec {
        const char* name;
        double massKg;
        double radiusKm;
        ThermalSpec thermal;
        OrbitalElements orbit;
    };

    constexpr double sunMassKg = 1.9885e30;
    constexpr double sunRadiusKm = 695700.0;
    constexpr double sunTempK = 5772.0;

    auto makeThermal = [](const ThermalSpec& spec, double internalHeatPower = 0.0) {
        ThermalProperties thermal;
        thermal.tempK = spec.tempK;
        thermal.internalHeatPower = internalHeatPower;
        thermal.referenceTempK = static_cast<float>(spec.tempK);
        thermal.emissivity = 1.0f;
        thermal.absorptivity = std::clamp(1.0f - spec.bondAlbedo, 0.0f, 1.0f);
        thermal.heatTransferCoeff = 0.0f;
        thermal.specificHeat = spec.specificHeat;
        thermal.thermalMassFraction = spec.thermalMassFraction;
        thermal.conductivity = spec.conductivity;
        thermal.density = spec.density;
        thermal.linearExpansionCoeff = spec.linearExpansionCoeff;
        thermal.meltingPoint = spec.meltingPoint;
        thermal.latentHeatFusion = spec.latentHeatFusion;
        thermal.boilingPoint = spec.boilingPoint;
        thermal.latentHeatVaporization = spec.latentHeatVaporization;
        return thermal;
    };

    auto emittedRadiationPower = [](double radiusKm, double tempK, double emissivity) {
        const double radiusM = radiusKm * 1000.0;
        const double surfaceArea = 4.0 * pi * radiusM * radiusM;
        return emissivity * Constants::STEFAN_BOLTZMANN * surfaceArea * std::pow(tempK, 4.0);
    };

    const double sunLuminosity = emittedRadiationPower(sunRadiusKm, sunTempK, 1.0);

    auto absorbedSolarPower = [&](double radiusKm, double orbitDistanceAu, double absorptivity) {
        const double radiusM = radiusKm * 1000.0;
        const double projectedArea = pi * radiusM * radiusM;
        const double orbitDistanceM = orbitDistanceAu * metersPerAu;
        const double irradiance = sunLuminosity / (4.0 * pi * orbitDistanceM * orbitDistanceM);
        return absorptivity * irradiance * projectedArea;
    };

    auto equilibriumInternalHeatPower = [&](double radiusKm, double orbitDistanceAu, const ThermalSpec& spec) {
        const double emitted = emittedRadiationPower(radiusKm, spec.tempK, 1.0);
        const double absorbed = absorbedSolarPower(radiusKm, orbitDistanceAu, 1.0 - static_cast<double>(spec.bondAlbedo));
        return std::max(0.0, emitted - absorbed);
    };

    auto orbitalPeriodSeconds = [&](const OrbitalElements& orbit, double centralMassKg) {
        const double semiMajorAxisM = orbit.semiMajorAxisAu * metersPerAu;
        return 2.0 * pi * std::sqrt((semiMajorAxisM * semiMajorAxisM * semiMajorAxisM) / (Constants::G * centralMassKg));
    };

    auto thermalSkinDepthM = [&](const ThermalSpec& spec, double forcingPeriodSeconds) {
        const double density = std::max(static_cast<double>(spec.density), 0.0);
        const double specificHeat = std::max(static_cast<double>(spec.specificHeat), 0.0);
        const double conductivity = std::max(static_cast<double>(spec.conductivity), 0.0);
        if (density <= 0.0 || specificHeat <= 0.0 || conductivity <= 0.0 || forcingPeriodSeconds <= 0.0) return 0.0;
        return std::sqrt((conductivity / (density * specificHeat)) * forcingPeriodSeconds / pi);
    };

    auto activeShellFraction = [&](double massKg, double radiusKm, const ThermalSpec& spec, double forcingPeriodSeconds) {
        const double radiusM = radiusKm * metersPerKm;
        const double depthM = std::min(thermalSkinDepthM(spec, forcingPeriodSeconds), radiusM);
        if (massKg <= 0.0 || radiusM <= 0.0 || depthM <= 0.0) return 0.0f;

        const double innerRadiusM = std::max(0.0, radiusM - depthM);
        const double shellVolume = (4.0 / 3.0) * pi * (radiusM * radiusM * radiusM - innerRadiusM * innerRadiusM * innerRadiusM);
        const double shellMass = shellVolume * static_cast<double>(spec.density);
        return static_cast<float>(std::clamp(shellMass / massKg, 1.0e-12, 1.0));
    };

    auto makeSolarThermal = [&](double massKg, double radiusKm, double orbitDistanceAu, double forcingPeriodSeconds, const ThermalSpec& spec) {
        ThermalSpec activeSpec = spec;
        activeSpec.thermalMassFraction = activeShellFraction(massKg, radiusKm, spec, forcingPeriodSeconds);
        return makeThermal(activeSpec, equilibriumInternalHeatPower(radiusKm, orbitDistanceAu, activeSpec));
    };

    auto makePlanetThermal = [&](double massKg, double radiusKm, const OrbitalElements& orbit, const ThermalSpec& spec) {
        return makeSolarThermal(massKg, radiusKm, orbit.semiMajorAxisAu, orbitalPeriodSeconds(orbit, sunMassKg), spec);
    };

    auto makeRockySpec = [](double tempK, float density, float bondAlbedo, float specificHeat, float conductivity) {
        return ThermalSpec{tempK, density, bondAlbedo, specificHeat, 0.0f, conductivity, 3.0e-5f, 1700.0f, 4.0e5f, 3000.0f, 1.0e7f};
    };

    auto makeFluidSpec = [](double tempK, float density, float bondAlbedo, float specificHeat, float conductivity) {
        return ThermalSpec{tempK, density, bondAlbedo, specificHeat, 0.0f, conductivity, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    };

    auto normalizeRadians = [](double angle) {
        constexpr double twoPi = 6.28318530717958647692528676655900576;
        angle = std::fmod(angle, twoPi);
        if (angle < -3.14159265358979323846264338327950288) angle += twoPi;
        if (angle > 3.14159265358979323846264338327950288) angle -= twoPi;
        return angle;
    };

    auto solveEccentricAnomaly = [&](double meanAnomalyRad, double eccentricity) {
        double e = meanAnomalyRad + eccentricity * std::sin(meanAnomalyRad);
        for (int i = 0; i < 12; ++i) {
            const double delta = (meanAnomalyRad - (e - eccentricity * std::sin(e))) / (1.0 - eccentricity * std::cos(e));
            e += delta;
            if (std::abs(delta) <= 1.0e-12) break;
        }
        return e;
    };

    auto rotateOrbitToEcliptic = [](const glm::dvec3& v, double omega, double node, double inclination) {
        const double cosOmega = std::cos(omega);
        const double sinOmega = std::sin(omega);
        const double cosNode = std::cos(node);
        const double sinNode = std::sin(node);
        const double cosI = std::cos(inclination);
        const double sinI = std::sin(inclination);

        return glm::dvec3(
            (cosOmega * cosNode - sinOmega * sinNode * cosI) * v.x + (-sinOmega * cosNode - cosOmega * sinNode * cosI) * v.y,
            (cosOmega * sinNode + sinOmega * cosNode * cosI) * v.x + (-sinOmega * sinNode + cosOmega * cosNode * cosI) * v.y,
            (sinOmega * sinI) * v.x + (cosOmega * sinI) * v.y
        );
    };

    auto eclipticToEngine = [](const glm::dvec3& v) {
        return glm::vec3(static_cast<float>(v.x), static_cast<float>(v.z), static_cast<float>(v.y));
    };

    auto stateFromElements = [&](const OrbitalElements& elements, double centralMassKg, double epochJd) {
        const double centuries = (epochJd - j2000Jd) / julianCenturyDays;
        const double a = (elements.semiMajorAxisAu + elements.semiMajorAxisAuPerCentury * centuries) * metersPerAu;
        const double e = elements.eccentricity + elements.eccentricityPerCentury * centuries;
        const double inclination = (elements.inclinationDeg + elements.inclinationDegPerCentury * centuries) * degToRad;
        const double meanLongitude = (elements.meanLongitudeDeg + elements.meanLongitudeDegPerCentury * centuries) * degToRad;
        const double longitudePerihelion = (elements.longitudePerihelionDeg + elements.longitudePerihelionDegPerCentury * centuries) * degToRad;
        const double node = (elements.longitudeAscendingNodeDeg + elements.longitudeAscendingNodeDegPerCentury * centuries) * degToRad;
        const double omega = longitudePerihelion - node;
        const double meanAnomaly = normalizeRadians(meanLongitude - longitudePerihelion);
        const double eccentricAnomaly = solveEccentricAnomaly(meanAnomaly, e);
        const double cosE = std::cos(eccentricAnomaly);
        const double sinE = std::sin(eccentricAnomaly);
        const double oneMinusECosE = 1.0 - e * cosE;
        const double sqrtOneMinusESq = std::sqrt(1.0 - e * e);
        const double meanMotion = std::sqrt(Constants::G * centralMassKg / (a * a * a));
        const double eccentricAnomalyRate = meanMotion / oneMinusECosE;

        const glm::dvec3 orbitalPosition(a * (cosE - e), a * sqrtOneMinusESq * sinE, 0.0);
        const glm::dvec3 orbitalVelocity(-a * sinE * eccentricAnomalyRate, a * sqrtOneMinusESq * cosE * eccentricAnomalyRate, 0.0);

        return OrbitalState{
            eclipticToEngine(rotateOrbitToEcliptic(orbitalPosition, omega, node, inclination)),
            eclipticToEngine(rotateOrbitToEcliptic(orbitalVelocity, omega, node, inclination))
        };
    };

    auto createSolarBody = [&](const BodySpec& spec) {
        const OrbitalState state = stateFromElements(spec.orbit, sunMassKg, defaultEpochJd);
        PointMassOptions options;
        options.base.position = state.position;
        options.base.scale = glm::vec3(static_cast<float>(spec.radiusKm * 2.0 * metersPerKm));
        options.mass = spec.massKg;
        options.velocity = state.velocity;

        SceneObject* body = sceneManager.createObject("prim_sphere", ResourceManager::getShader("basic"), options);
        sceneManager.setObjectName(body, spec.name);
        body->getPhysicsBody()->setThermalProperty(makePlanetThermal(spec.massKg, spec.radiusKm, spec.orbit, spec.thermal), BodyLock::NOLOCK);
        return body;
    };

    sceneManager.setGlobalAcceleration(glm::vec3(0.0f));
    sceneManager.setAmbientTemperature(2.725f);
    sceneManager.setGravitationalConstant(Constants::G);
    sceneManager.setSimSpeed(1.0e6f);

    PointMassOptions sunOptions;
    sunOptions.base.position = glm::vec3(0.0f);
    sunOptions.base.scale = glm::vec3(static_cast<float>(sunRadiusKm * 2.0 * metersPerKm));
    sunOptions.mass = sunMassKg;
    SceneObject* sun = sceneManager.createObject("prim_sphere", ResourceManager::getShader("basic"), sunOptions);
    sceneManager.setObjectName(sun, "Sun");
    sun->getPhysicsBody()->setThermalProperty(makeThermal(makeFluidSpec(sunTempK, 1408.0f, 0.0f, 20780.0f, 1.0e4f), sunLuminosity), BodyLock::NOLOCK);

    const std::array<BodySpec, 8> planets{{
        {"Mercury", 0.33010e24, 2439.7, makeRockySpec(440.15, 5429.0f, 0.068f, 800.0f, 2.0f), {0.38709927, 0.00000037, 0.20563593, 0.00001906, 7.00497902, -0.00594749, 252.25032350, 149472.67411175, 77.45779628, 0.16047689, 48.33076593, -0.12534081}},
        {"Venus",   4.8673e24, 6051.8, makeRockySpec(737.15, 5243.0f, 0.770f, 850.0f, 2.0f), {0.72333566, 0.00000390, 0.00677672, -0.00004107, 3.39467605, -0.00078890, 181.97909950, 58517.81538729, 131.60246718, 0.00268329, 76.67984255, -0.27769418}},
        {"Earth",   5.9722e24, 6371.0, makeRockySpec(288.15, 5513.0f, 0.294f, 1000.0f, 2.5f), {1.00000261, 0.00000562, 0.01671123, -0.00004392, -0.00001531, -0.01294668, 100.46457166, 35999.37244981, 102.93768193, 0.32327364, 0.0, 0.0}},
        {"Mars",    0.64169e24, 3389.5, makeRockySpec(208.15, 3934.0f, 0.250f, 750.0f, 0.08f), {1.52371034, 0.00001847, 0.09339410, 0.00007882, 1.84969142, -0.00813131, -4.55343205, 19140.30268499, -23.94362959, 0.44441088, 49.55953891, -0.29257343}},
        {"Jupiter", 1898.13e24, 69911.0, makeFluidSpec(163.15, 1326.0f, 0.343f, 13000.0f, 0.18f), {5.20288700, -0.00011607, 0.04838624, -0.00013253, 1.30439695, -0.00183714, 34.39644051, 3034.74612775, 14.72847983, 0.21252668, 100.47390909, 0.20469106}},
        {"Saturn",  568.32e24, 58232.0, makeFluidSpec(133.15, 687.0f, 0.342f, 13000.0f, 0.18f), {9.53667594, -0.00125060, 0.05386179, -0.00050991, 2.48599187, 0.00193609, 49.95424423, 1222.49362201, 92.59887831, -0.41897216, 113.66242448, -0.28867794}},
        {"Uranus",  86.811e24, 25362.0, makeFluidSpec(78.15, 1270.0f, 0.300f, 9000.0f, 0.6f), {19.18916464, -0.00196176, 0.04725744, -0.00004397, 0.77263783, -0.00242939, 313.23810451, 428.48202785, 170.95427630, 0.40805281, 74.01692503, 0.04240589}},
        {"Neptune", 102.409e24, 24622.0, makeFluidSpec(73.15, 1638.0f, 0.290f, 9000.0f, 0.6f), {30.06992276, 0.00026291, 0.00859048, 0.00005105, 1.77004347, 0.00035372, -55.12002969, 218.45945325, 44.96476227, -0.32241464, 131.78422574, -0.00508664}},
    }};

    SceneObject* earth = nullptr;
    OrbitalState earthState{};
    for (const BodySpec& planet : planets) {
        SceneObject* body = createSolarBody(planet);
        if (std::string_view(planet.name) == "Earth") {
            earth = body;
            earthState.position = body->getPosition();
            earthState.velocity = body->getPhysicsBody()->getVelocity(BodyLock::NOLOCK);
        }
    }

    const OrbitalState moonRelativeState = stateFromElements(
        {0.002569555, 0.0, 0.0549, 0.0, 5.1454, 0.0, 558.5516, 481267.88088047254, 443.1862, 4069.01334885, 125.1228, -1934.1378481575},
        5.9724e24,
        defaultEpochJd
    );
    PointMassOptions moonOptions;
    moonOptions.base.position = earthState.position + moonRelativeState.position;
    moonOptions.base.scale = glm::vec3(1737.4f * 2.0f * static_cast<float>(metersPerKm));
    moonOptions.mass = 0.07346e24;
    moonOptions.velocity = earthState.velocity + moonRelativeState.velocity;
    SceneObject* moon = sceneManager.createObject("prim_sphere", ResourceManager::getShader("basic"), moonOptions);
    sceneManager.setObjectName(moon, "Moon");
    const double earthYearSeconds = orbitalPeriodSeconds({1.00000261, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, sunMassKg);
    moon->getPhysicsBody()->setThermalProperty(makeSolarThermal(0.07346e24, 1737.4, 1.00000261, earthYearSeconds, makeRockySpec(270.4, 3344.0f, 0.110f, 741.0f, 0.001f)), BodyLock::NOLOCK);

    sceneManager.focusObject(earth);
}

}
