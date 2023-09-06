#ifdef __cplusplus
#pragma once
using glm::mat4;
using glm::vec3;
using glm::vec4;
#else
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

const float PI = 3.1415926535;
#endif

#ifdef __cplusplus
struct PushConstants {
    glm::mat4 invView;
    glm::mat4 invProj;

    int frame = 0;
    int sampleCount = 10;
    int enableAccum = 1;
    float domeLightPhi = 0.0f;

    uint64_t instanceDataAddress;
};
#else
layout(push_constant) uniform PushConstants {
    mat4 invView;
    mat4 invProj;

    int frame;
    int sampleCount;
    int enableAccum;
    float domeLightPhi;

    uint64_t instanceDataAddress;
};
#endif

// shared
struct InstanceData {
    mat4 transformMatrix;
    mat4 normalMatrix;

    int meshIndex;

    int baseColorTextureIndex;
    int metallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;

    float metallicFactor;
    float roughnessFactor;
    vec4 baseColorFactor;
    vec3 emissiveFactor;
};

#ifndef __cplusplus

struct HitPayload {
    vec3 radiance;
    int depth;
    uint seed;
};

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
};

// Image
layout(binding = 0, rgba8) uniform image2D baseImage;
layout(binding = 1) uniform sampler2D domeLightTexture;

// Accel
layout(binding = 10) uniform accelerationStructureEXT topLevelAS;

// Buffer
layout(binding = 20) buffer VertexBuffers {
    Vertex vertices[];
}
vertexBuffers[];

layout(binding = 21) buffer IndexBuffers {
    uint indices[];
}
indexBuffers[];

layout(buffer_reference, scalar) buffer InstanceDataBuffer {
    InstanceData instanceData[];
};

#endif
