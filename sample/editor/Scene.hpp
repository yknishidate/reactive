#pragma once
#include <reactive/Scene/Camera.hpp>
#include <reactive/Scene/Mesh.hpp>

struct Material {
    glm::vec4 baseColor{1.0f};
    glm::vec3 emissive{0.0f};
    float metallic{0.0f};
    float roughness{0.0f};
    float ior{1.5f};

    int baseColorTextureIndex{-1};
    int metallicRoughnessTextureIndex{-1};
    int normalTextureIndex{-1};
    int occlusionTextureIndex{-1};
    int emissiveTextureIndex{-1};
    std::string name;
};

struct Transform {
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};

    glm::mat4 computeTransformMatrix() const {
        glm::mat4 T = glm::translate(glm::mat4{1.0}, translation);
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 S = glm::scale(glm::mat4{1.0}, scale);
        return T * R * S;
    }

    glm::mat4 computeNormalMatrix() const {
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 S = glm::scale(glm::mat4{1.0}, glm::vec3{1.0} / scale);
        return R * S;
    }

    static Transform lerp(const Transform& a, const Transform& b, float t) {
        Transform transform;
        transform.translation = glm::mix(a.translation, b.translation, t);
        transform.rotation = glm::lerp(a.rotation, b.rotation, t);
        transform.scale = glm::mix(a.scale, b.scale, t);
        return transform;
    }
};

struct KeyFrame {
    int frame;
    Transform transform;
};

class Node {
public:
    Transform computeTransformAtFrame(int frame) const {
        // Handle frame out of range
        if (frame <= keyFrames.front().frame) {
            return keyFrames.front().transform;
        }
        if (frame >= keyFrames.back().frame) {
            return keyFrames.back().transform;
        }

        // Search frame
        for (int i = 0; i < keyFrames.size(); i++) {
            const auto& keyFrame = keyFrames[i];
            if (keyFrame.frame == frame) {
                return keyFrame.transform;
            }

            if (keyFrame.frame > frame) {
                const KeyFrame& prev = keyFrames[i - 1];
                const KeyFrame& next = keyFrames[i];
                float t = (frame = prev.frame) / (next.frame - prev.frame);

                return Transform::lerp(prev.transform, next.transform, t);
            }
        }
    }

    glm::mat4 computeTransformMatrix(int frame) const {
        if (keyFrames.empty()) {
            return transform.computeTransformMatrix();
        }
        return computeTransformAtFrame(frame).computeTransformMatrix();
    }

    glm::mat4 computeNormalMatrix(int frame) const {
        if (keyFrames.empty()) {
            return transform.computeNormalMatrix();
        }
        return computeTransformAtFrame(frame).computeNormalMatrix();
    }

    std::string name;
    rv::Mesh* mesh = nullptr;
    Material* material = nullptr;
    Transform transform;
    std::vector<KeyFrame> keyFrames;
};

class Scene {
public:
    std::vector<Node> nodes;
    std::vector<rv::Mesh> meshes;
    std::vector<Material> materials;
    std::vector<rv::Camera> cameras;
};
