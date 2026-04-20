#include "AppSettings.h"

void AppSettings::load(QSettings& settings) {
    for (auto& group : groups) {
        group->load(settings);
    }
}

void AppSettings::save(QSettings& settings) const {
    for (const auto& group : groups) {
        group->save(settings);
    }
}
