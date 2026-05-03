#pragma once

#include <cstddef>
#include <QString>
#include <QObject>

#include "physics/PhysicsBody.h"

enum class Metric {
    PositionX,
    PositionY,
    PositionZ,
    VelocityX,
    VelocityY,
    VelocityZ,
    Temperature,

    Count, // Must be last (just dont have to hard code the count elsewhere)
};

inline constexpr std::size_t kPlottableMetricCount = static_cast<std::size_t>(Metric::Count);

inline QString metricLabel(Metric metric) {
    switch (metric) {
        case Metric::PositionX:     return QObject::tr("Position X");
        case Metric::PositionY:     return QObject::tr("Position Y");
        case Metric::PositionZ:     return QObject::tr("Position Z");
        case Metric::VelocityX:     return QObject::tr("Velocity X");
        case Metric::VelocityY:     return QObject::tr("Velocity Y");
        case Metric::VelocityZ:     return QObject::tr("Velocity Z");
        case Metric::Temperature:   return QObject::tr("Temperature (K)");
        case Metric::Count:         return QObject::tr("Invalid Metric");
    }
    return QObject::tr("Unknown Metric");
}

inline float objectSnapshotValue(Metric metric, const ObjectSnapshot& s) {
    switch (metric) {
        case Metric::PositionX:     return s.position.x;
        case Metric::PositionY:     return s.position.y;
        case Metric::PositionZ:     return s.position.z;
        case Metric::VelocityX:     return s.velocity.x;
        case Metric::VelocityY:     return s.velocity.y;
        case Metric::VelocityZ:     return s.velocity.z;
        case Metric::Temperature:   return s.temperature;
        case Metric::Count:         return 0.0f;
    }
    return 0.0f;
}
