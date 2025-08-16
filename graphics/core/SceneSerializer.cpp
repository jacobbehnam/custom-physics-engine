#include "SceneSerializer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include "graphics/core/SceneObject.h"

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
        objJson["meshName"] = obj->getMeshName().c_str();

        QJsonObject optionsJson;
        std::visit([&](auto&& opt){
            using T = std::decay_t<decltype(opt)>;
            QJsonObject data;
            data["position"] = JsonUtils::vec3ToJson(obj->getPosition());
            data["scale"] = JsonUtils::vec3ToJson(obj->getScale());
            data["rotation"] = JsonUtils::vec3ToJson(obj->getRotation());

            if constexpr (std::is_same_v<T, PointMassOptions>) {
                data["isStatic"] = obj->getPhysicsBody()->getIsStatic(BodyLock::LOCK);
                data["mass"] = obj->getPhysicsBody()->getMass(BodyLock::LOCK);
                optionsJson["type"] = "PointMassOptions";
            }
            else if constexpr (std::is_same_v<T, RigidBodyOptions>) {
                data["isStatic"] = obj->getPhysicsBody()->getIsStatic(BodyLock::LOCK);
                data["mass"] = obj->getPhysicsBody()->getMass(BodyLock::LOCK);
                optionsJson["type"] = "RigidBodyOptions";
            }
            else {
                optionsJson["type"] = "ObjectOptions";
            }

            optionsJson["data"] = data;
        }, obj->getCreationOptions());

        objJson["options"] = optionsJson;
        objectsArray.append(objJson);
    }
    root["objects"] = objectsArray;

    QJsonDocument doc(root);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for saving:" << filename;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool SceneSerializer::loadFromJson(const QString &filename) {

}