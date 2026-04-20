#pragma once

class ISettingsTab {
public:
    virtual ~ISettingsTab() = default;
    virtual void saveSettings() = 0;
};
