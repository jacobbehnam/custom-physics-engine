#include "SceneSerializer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

namespace JsonUtils {
    // Helper to convert glm::vec3 → QJsonArray
    inline QJsonArray vec3ToJson(const glm::vec3 &v) {
        QJsonArray arr;
        arr.append(v.x);
        arr.append(v.y);
        arr.append(v.z);
        return arr;
    }
}

SceneSerializer::SceneSerializer(SceneManager *sceneMgr) : sceneManager(sceneMgr) {}

bool SceneSerializer::saveToJson(const QString &filename) const {
    QJsonObject root;

    root["engineVersion"] = "1.0.0";

    // Global settings
    QJsonObject settings;
    settings["gravity"] = JsonUtils::vec3ToJson(sceneManager->getGlobalAcceleration());
    settings["simSpeed"] = sceneManager->getSimSpeed();
    root["settings"] = settings;

    QJsonArray objectsArray;
    for (auto* obj : sceneManager->getObjects()) {
        QJsonObject objJson;
        objJson["id"] = static_cast<double>(obj->getObjectID());

    }
}

bool SceneSerializer::loadFromJson(const QString &filename) {

}