#pragma once
#include <QSettings>

class ISettingsGroup {
public:
    virtual ~ISettingsGroup() = default;
    
    virtual void load(QSettings& settings) = 0;
    virtual void save(QSettings& settings) const = 0;
};
