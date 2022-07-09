#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : enable

precision highp float;

const highp float M_PI = 3.14159265358979323846;

struct HitPayload
{
    vec3 position;
    vec3 normal;
    vec3 diffuse;
    vec3 emission;
    int instanceIndex;
};

#include "../../share/structure.glsl"

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
layout(buffer_reference, scalar) buffer Objects { ObjectData o[]; };
layout(binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 1, rgba16f) uniform image2D position;
layout(binding = 2, rgba16f) uniform image2D normal;
layout(binding = 3, rgba16f) uniform image2D diffuse;
layout(binding = 4, rgba16f) uniform image2D emission;
layout(binding = 5, rgba8ui) uniform uimage2D instanceIndex;

layout(binding = 6) buffer Addresses { BufferAddress address[]; } addresses;
layout(binding = 7) uniform sampler2D samplers[];
