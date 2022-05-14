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
    vec3 weight;
    uint seed;
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
layout(binding = 5, set = 0, r16ui) uniform uimage2D reservoirSampleImage;
layout(binding = 6, set = 0, r32f) uniform image2D reservoirWeightImage;
layout(binding = 7, set = 0, rgba32f) uniform image2D posImage;
layout(binding = 8, set = 0, rgba32f) uniform image2D normalImage;
layout(binding = 9, set = 0, rg16ui) uniform uimage2D indexImage;
layout(binding = 10, set = 0, rgba16f) uniform image2D diffuseImage;
layout(binding = 11, set = 0, rgba16f) uniform image2D emissionImage;
layout(binding = 3) buffer Addresses { BufferAddress address[]; } addresses;
layout(binding = 4) uniform sampler2D samplers[];

void createCoordinateSystem(in vec3 N, out vec3 T, out vec3 B)
{
    if (abs(N.x) > abs(N.y))
        T = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
    else
        T = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
    B = cross(N, T);
}

vec3 sampleHemisphere(in float rand1, in float rand2)
{
    vec3 dir;
    dir.x = cos(2 * M_PI * rand2) * sqrt(1 - rand1 * rand1);
    dir.y = sin(2 * M_PI * rand2) * sqrt(1 - rand1 * rand1);
    dir.z = rand1;
    return dir;
}
