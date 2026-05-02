#include "SceneSerializer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

#include "ResourceManager.h"
#include "graphics/core/SceneObject.h"

namespace JsonUtils {
    inline double numberOr(const QJsonObject& obj, const char* key, double fallback) {
        const QJsonValue value = obj.value(key);
        return value.isDouble() ? value.toDouble(fallback) : fallback;
    }

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

    inline QJsonObject thermalToJson(const ThermalProperties& props) {
        QJsonObject obj;
        obj["tempK"] = props.tempK;
        obj["internalHeatPower"] = props.internalHeatPower;
        obj["externalHeatFlux"] = props.externalHeatFlux;
        obj["specificHeat"] = props.specificHeat;
        obj["thermalMassFraction"] = props.thermalMassFraction;
        obj["emissivity"] = props.emissivity;
        obj["absorptivity"] = props.absorptivity;
        obj["heatTransferCoeff"] = props.heatTransferCoeff;
        obj["conductivity"] = props.conductivity;
        obj["density"] = props.density;
        obj["meltingPoint"] = props.meltingPoint;
        obj["latentHeatFusion"] = props.latentHeatFusion;
        obj["fusionProgress"] = props.fusionProgress;
        obj["boilingPoint"] = props.boilingPoint;
        obj["latentHeatVaporization"] = props.latentHeatVaporization;
        obj["vaporizationProgress"] = props.vaporizationProgress;
        return obj;
    }

    inline ThermalProperties jsonToThermal(const QJsonObject& obj, const ThermalProperties& fallback) {
        ThermalProperties props = fallback;
        props.tempK = numberOr(obj, "tempK", props.tempK);
        props.internalHeatPower = numberOr(obj, "internalHeatPower", props.internalHeatPower);
        props.externalHeatFlux = numberOr(obj, "externalHeatFlux", props.externalHeatFlux);
        props.specificHeat = static_cast<float>(numberOr(obj, "specificHeat", props.specificHeat));
        props.thermalMassFraction = static_cast<float>(numberOr(obj, "thermalMassFraction", props.thermalMassFraction));
        props.emissivity = static_cast<float>(numberOr(obj, "emissivity", props.emissivity));
        props.absorptivity = static_cast<float>(numberOr(obj, "absorptivity", props.absorptivity));
        props.heatTransferCoeff = static_cast<float>(numberOr(obj, "heatTransferCoeff", props.heatTransferCoeff));
        props.conductivity = static_cast<float>(numberOr(obj, "conductivity", props.conductivity));
        props.density = static_cast<float>(numberOr(obj, "density", props.density));
        props.meltingPoint = static_cast<float>(numberOr(obj, "meltingPoint", props.meltingPoint));
        props.latentHeatFusion = static_cast<float>(numberOr(obj, "latentHeatFusion", props.latentHeatFusion));
        props.fusionProgress = static_cast<float>(numberOr(obj, "fusionProgress", props.fusionProgress));
        props.boilingPoint = static_cast<float>(numberOr(obj, "boilingPoint", props.boilingPoint));
        props.latentHeatVaporization = static_cast<float>(numberOr(obj, "latentHeatVaporization", props.latentHeatVaporization));
        props.vaporizationProgress = static_cast<float>(numberOr(obj, "vaporizationProgress", props.vaporizationProgress));
        return props;
    }
}

SceneSerializer::SceneSerializer(SceneManager *sceneMgr) : sceneManager(sceneMgr) {}

bool SceneSerializer::saveToJson(const QString &filename) const {
    QJsonObject root;

    root["engineVersion"] = "1.0.0";

    // Global settings
    QJsonObject settings;
    settings["gravity"] = JsonUtils::vec3ToJson(sceneManager->getGlobalAcceleration());
    settings["gravitationalConstant"] = sceneManager->getGravitationalConstant();
    settings["simSpeed"] = sceneManager->getSimSpeed();
    settings["ambientTemperature"] = sceneManager->getAmbientTemperature();
    root["settings"] = settings;

    QJsonArray objectsArray;
    for (auto* obj : sceneManager->getObjects()) {
        QJsonObject objJson;
        objJson["id"] = static_cast<double>(obj->getObjectID());
        objJson["meshName"] = QString::fromStdString(obj->getMeshName());
        objJson["name"] = QString::fromStdString(obj->getName());
        objJson["shader"] = QString::fromStdString(ResourceManager::getShaderName(obj->getShader()));

        QJsonObject optionsJson;
        std::visit([&](auto&& opt){
            using T = std::decay_t<decltype(opt)>;
            QJsonObject data;
            data["position"] = JsonUtils::vec3ToJson(obj->getPosition());
            data["scale"] = JsonUtils::vec3ToJson(obj->getScale());
            data["rotation"] = JsonUtils::vec3ToJson(obj->getRotation());
            data["isStatic"] = obj->getPhysicsBody()->getIsStatic(BodyLock::LOCK);
            data["mass"] = obj->getPhysicsBody()->getMass(BodyLock::LOCK);
            data["velocity"] = JsonUtils::vec3ToJson(obj->getPhysicsBody()->getVelocity(BodyLock::LOCK));
            data["thermal"] = JsonUtils::thermalToJson(obj->getPhysicsBody()->getThermalProperties(BodyLock::LOCK));

            if constexpr (std::is_same_v<T, PointMassOptions>) {
                optionsJson["type"] = "PointMassOptions";
            }
            else if constexpr (std::is_same_v<T, RigidBodyOptions>) {
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
        if (settings["gravity"].isArray()) {
            sceneManager->setGlobalAcceleration(JsonUtils::jsonToVec3(settings["gravity"].toArray()));
        }
        sceneManager->setSimSpeed(JsonUtils::numberOr(settings, "simSpeed", sceneManager->getSimSpeed()));
        sceneManager->setGravitationalConstant(JsonUtils::numberOr(settings, "gravitationalConstant", sceneManager->getGravitationalConstant()));
        sceneManager->setAmbientTemperature(static_cast<float>(JsonUtils::numberOr(settings, "ambientTemperature", sceneManager->getAmbientTemperature())));
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
            std::string objName = objJson["name"].toString().toStdString();
            std::string shaderName = objJson["shader"].toString().toStdString();

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
                    pointOpt.velocity = JsonUtils::jsonToVec3(data["velocity"].toArray());
                    options = pointOpt;
                }
                else if (type == "RigidBodyOptions") {
                    // Needs collider so have to use this method
                    options = RigidBodyOptions::Box(
                        base, 
                        data["isStatic"].toBool(),
                        data["mass"].toDouble(),
                        JsonUtils::jsonToVec3(data["velocity"].toArray())
                    );
                }
                else {
                    options = base;
                }
            }
            Shader* shader = ResourceManager::getShader(shaderName);
            SceneObject* createObj = sceneManager->createObject(meshName, shader, options);
            sceneManager->setObjectName(createObj, objName);
            if (createObj->getPhysicsBody() && objJson["options"].isObject()) {
                QJsonObject optionsJson = objJson["options"].toObject();
                QJsonObject data = optionsJson["data"].toObject();
                if (data["thermal"].isObject()) {
                    ThermalProperties fallback = createObj->getPhysicsBody()->getThermalProperties(BodyLock::LOCK);
                    createObj->getPhysicsBody()->setThermalProperty(JsonUtils::jsonToThermal(data["thermal"].toObject(), fallback), BodyLock::LOCK);
                }
            }
        }
    }
    return true;
}
