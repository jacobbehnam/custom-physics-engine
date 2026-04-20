#pragma once

#include <QSettings>
#include <vector>
#include <memory>
#include <stdexcept>
#include "settings/ISettingsGroup.h"

class AppSettings {
public:
    static AppSettings& getInstance() {
        static AppSettings instance;
        return instance;
    }

    template<typename T, typename... Args>
    T* registerGroup(Args&&... args) {
        auto group = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = group.get();
        groups.push_back(std::move(group));
        return ptr;
    }

    // Not worth right now
    // to use a map, the overhead will be more 
    // computation than just iterating through the vector and dynamic casting
    template<typename T>
    T& getGroup() const {
        for (const auto& group : groups) {
            if (auto* casted = dynamic_cast<T*>(group.get())) {
                return *casted;
            }
        }
        throw std::runtime_error("Settings group not found!");
    }

    void load(QSettings& settings);
    void save(QSettings& settings) const;

private:
    AppSettings() = default;
    ~AppSettings() = default;
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;

    std::vector<std::unique_ptr<ISettingsGroup>> groups;
};
