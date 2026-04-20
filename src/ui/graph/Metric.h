#pragma once

#include <QString>
#include <QObject>

enum class Metric {
    PositionX,
    PositionY,
    PositionZ,
    VelocityX,
    VelocityY,
    VelocityZ,

    Count, // Must be last (just dont have to hard code the count elsewhere)
};

inline QString metricLabel(Metric metric) {
    switch (metric) {
        case Metric::PositionX: return QObject::tr("Position X");
        case Metric::PositionY: return QObject::tr("Position Y");
        case Metric::PositionZ: return QObject::tr("Position Z");
        case Metric::VelocityX: return QObject::tr("Velocity X");
        case Metric::VelocityY: return QObject::tr("Velocity Y");
        case Metric::VelocityZ: return QObject::tr("Velocity Z");
        case Metric::Count:     return QObject::tr("Invalid Metric");
    }
    return QObject::tr("Unknown Metric");
}
