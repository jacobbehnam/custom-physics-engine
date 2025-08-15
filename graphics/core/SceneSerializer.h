#pragma once
#include <graphics/core/SceneManager.h>

class SceneSerializer {
public:
    explicit SceneSerializer(SceneManager* sceneMgr);

    bool saveToJson(const QString& filename) const;
    bool loadFromJson(const QString& filename);

private:
    SceneManager* sceneManager;
};