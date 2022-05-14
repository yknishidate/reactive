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
    vec3 emission;
    vec3 brdf;
    vec3 color;
    bool done;
    bool skip;
};

#include "../../share/structure.inc"

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
layout(buffer_reference, scalar) buffer Objects { ObjectData o[]; };
layout(buffer_reference, scalar) buffer PointLights { PointLight p[]; };
layout(buffer_reference, scalar) buffer SphereLights { SphereLight s[]; };
layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D inputImage;
layout(binding = 2, set = 0, rgba8) uniform image2D outputImage;
layout(binding = 5, set = 0, r16) uniform image2D reservoirSampleImage;
layout(binding = 6, set = 0, r16f) uniform image2D reservoirWeightImage;
layout(binding = 6, set = 0, rgba8) uniform image2D objectImage;
layout(binding = 3) buffer Addresses { BufferAddress address[]; } addresses;
layout(binding = 4) uniform sampler2D samplers[];
