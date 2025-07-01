#include "TranslateHandle.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "graphics/core/SceneObject.h"
#include <graphics/utils/MathUtils.h>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "graphics/core/ResourceManager.h"

// Expects triangulated obj files!
static bool loadOBJ(const std::string& path, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;

    std::unordered_map<Vertex, unsigned int> vertexToIndex;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (type == "vt") {
            // glm::vec2 uv;
            // iss >> uv.x >> uv.y;
            // texCoords.push_back(uv);
        } else if (type == "vn") {
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        } else if (type == "f") {
            std::string vertexStr;
            while (iss >> vertexStr) {
                std::istringstream vss(vertexStr);
                std::string idxStr;
                int posIdx = 0, uvIdx = 0, normIdx = 0;

                std::getline(vss, idxStr, '/');
                posIdx = std::stoi(idxStr);

                if (std::getline(vss, idxStr, '/') && !idxStr.empty())
                    uvIdx = std::stoi(idxStr);
                if (std::getline(vss, idxStr, '/'))
                    normIdx = std::stoi(idxStr);

                Vertex vertex{};
                vertex.pos = positions[posIdx - 1];
                //if (uvIdx) vertex.texCoord = texCoords[uvIdx - 1];
                if (normIdx) vertex.normal = normals[normIdx - 1];

                if (vertexToIndex.count(vertex) == 0) {
                    vertexToIndex[vertex] = static_cast<unsigned int>(outVertices.size());
                    outVertices.push_back(vertex);
                }

                outIndices.push_back(vertexToIndex[vertex]);
            }
        }
    }

    return true;
}


static glm::vec3 axisDir(Axis a) {
    switch (a) {
        case Axis::X:
            return {1,0,0};
        case Axis::Y:
            return {0,1,0};
        default:
            return {0,0,1};
    }
}

TranslateHandle::TranslateHandle(Mesh *m, Shader* sdr, SceneObject *tgt, Axis ax)
    : mesh(m), shader(sdr), target(tgt), axis(ax) {
    std::vector<Vertex> outV;
    std::vector<unsigned int> outI;
    if (loadOBJ("../Arrow.obj", outV, outI)) {
        mesh = ResourceManager::LoadMesh(outV, outI, "arrow");
    } else {
        std::cout << "Unable to load obj" << std::endl;
    }
}

glm::mat4 TranslateHandle::getModelMatrix() const {
    glm::mat4 model(1.0f);
    glm::vec3 pos = target->getPosition();
    model = glm::translate(model, pos);

    glm::vec3 dir = axisDir(axis);
    glm::vec3 from = {0,1,0}, to = dir; // We assume the TranslateHandle mesh points in the +Y axis direction.
    float cosA = glm::dot(from, to);
    glm::vec3 crossA = glm::cross(from, to);
    if (glm::length(crossA) > 1e-3f) {
        float angle = acos(glm::clamp(cosA, -1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::normalize(crossA));
    }

    model = glm::scale(model, glm::vec3(thickness, length, thickness));
    // model = glm::translate(model, glm::vec3(1.0f, 1.0f, 1.0f));
    return model;
}

void TranslateHandle::onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) {
    glm::vec3 axisDirection = axisDir(axis);
    // To solve for t, minimize the quantity || (rayOrig + rayDir * t) - initialHitPoint ||
    float t = glm::dot(-(rayOrig - initialHitPoint), rayDir);

    glm::vec3 delta = (rayOrig + rayDir * t) - initialHitPoint;
    // apply translation only along the axis direction component:
    float moveAmount = glm::dot(delta, axisDirection);
    target->setPosition(originalPosition + axisDirection * moveAmount);
}


void TranslateHandle::draw() const {
    shader->use();

    glm::mat4 model = getModelMatrix();
    shader->setMat4("model", model);

    mesh->draw();
}

Shader* TranslateHandle::getShader() const {
    return shader;
}

Mesh* TranslateHandle::getMesh() const {
    return mesh;
}

void TranslateHandle::setDragState(glm::vec3 initHitPos, glm::vec3 originPos) {
    initialHitPoint = initHitPos;
    originalPosition = originPos;
}
