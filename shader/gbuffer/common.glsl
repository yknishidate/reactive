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

#include "../../share/structure.inc"

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
layout(buffer_reference, scalar) buffer Objects { ObjectData o[]; };
layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding =  7, set = 0, rgba32f) uniform image2D position;
layout(binding =  8, set = 0, rgba32f) uniform image2D normal;
layout(binding = 10, set = 0, rgba16f) uniform image2D diffuse;
layout(binding = 11, set = 0, rgba16f) uniform image2D emission;
layout(binding =  9, set = 0, rg16ui) uniform uimage2D instanceIndex;

layout(binding = 3) buffer Addresses { BufferAddress address[]; } addresses;
layout(binding = 4) uniform sampler2D samplers[];
