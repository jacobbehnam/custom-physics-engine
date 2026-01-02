#include "SceneSerializer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

#include "ResourceManager.h"
#include "graphics/core/SceneObject.h"

namespace JsonUtils {
    inline QJsonArray vec3ToJson(const glm::vec3 &v) {
        QJsonArray arr;
        arr.append(v.x);
        arr.append(v.y);
        arr.append(v.z);
        return arr;
    }

    inline glm::vec3 jsonToVec3(QJsonArray arr) {
        if (arr.size() != 3) {
            //qWarning << "QJsonArray does not have 3 elements to cast to vec3";
            return glm::vec3(0.0f);
        }
        if (!(arr[0].isDouble() || arr[1].isDouble() || arr[2].isDouble())) {
            //qWarning << "Unable to cast to vec3: type is not double";
            return glm::vec3(0.0f);
        }
        return {arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble()};
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
        //objJson["meshName"] = obj->getMeshName().c_str();

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
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for loading:" << filename;
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON format in file:" << filename;
        return false;
    }

    QJsonObject root = doc.object();
    // Can check engine version for compatibility

    if (root.contains("settings") && root["settings"].isObject()) {
        QJsonObject settings = root["settings"].toObject();
        sceneManager->setGlobalAcceleration(JsonUtils::jsonToVec3(settings["gravity"].toArray()));
        sceneManager->setSimSpeed(settings["simSpeed"].toDouble());
    }

    sceneManager->deleteAllObjects();
    sceneManager->setSelectFor(nullptr);

    if (root.contains("objects") && root["objects"].isArray()) {
        QJsonArray objectsArray = root["objects"].toArray();
        for (const auto &value : objectsArray) {
            if (!value.isObject()) continue;
            QJsonObject objJson = value.toObject();

            uint32_t id = static_cast<uint32_t>(objJson["id"].toDouble());
            std::string meshName = objJson["meshName"].toString().toStdString();

            CreationOptions options;
            if (objJson.contains("options") && objJson["options"].isObject()) {
                QJsonObject optionsJson = objJson["options"].toObject();
                QString type = optionsJson["type"].toString();
                QJsonObject data = optionsJson["data"].toObject();

                ObjectOptions base;
                base.position = JsonUtils::jsonToVec3(data["position"].toArray());
                base.scale    = JsonUtils::jsonToVec3(data["scale"].toArray());
                base.rotation = JsonUtils::jsonToVec3(data["rotation"].toArray());

                if (type == "PointMassOptions") {
                    PointMassOptions pointOpt;
                    pointOpt.base = base;
                    pointOpt.isStatic = data["isStatic"].toBool();
                    pointOpt.mass = data["mass"].toDouble();
                    options = pointOpt;
                }
                else if (type == "RigidBodyOptions") {
                    RigidBodyOptions rigidOpt;
                    rigidOpt.base = base;
                    rigidOpt.isStatic = data["isStatic"].toBool();
                    rigidOpt.mass = data["mass"].toDouble();
                    options = rigidOpt;
                }
                else {
                    options = base;
                }
            }
            sceneManager->createObject(meshName, ResourceManager::getShader("basic"), options);
        }
    }
    return true;
}