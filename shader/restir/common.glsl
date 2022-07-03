#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : enable
#include "../random.glsl"

precision highp float;

const highp float M_PI = 3.14159265358979323846;

struct HitPayload
{
    bool shadowed;
};

struct Reservoir
{
    float w_sum; // sum of all weights
    float p_z;   // probability of target
    int x_z;     // sample
    int m;       // number of streaming
};

bool updateReservoir(inout Reservoir reservoir, int x, float w, float p, inout uint seed)
{
    reservoir.w_sum += w;
    reservoir.m += 1;
    if (rand(seed) < (w / reservoir.w_sum)) {
        reservoir.x_z = x;
        reservoir.p_z = p;
        return true;
    }
    return false;
}

float calcReservoirWeight(in Reservoir reservoir)
{
    return reservoir.w_sum / reservoir.m / reservoir.p_z;
}

#include "../../share/structure.inc"

layout(buffer_reference, scalar) buffer SphereLights { SphereLight s[]; };
layout(binding =  0) uniform accelerationStructureEXT topLevelAS;
layout(binding =  1, rgba8) uniform image2D inputImage;
layout(binding =  2, rgba8) uniform image2D outputImage;
layout(binding =  7, rgba16f) uniform image2D positionImage;
layout(binding =  8, rgba16f) uniform image2D normalImage;
layout(binding = 10, rgba16f) uniform image2D diffuseImage;
layout(binding = 11, rgba16f) uniform image2D emissionImage;
layout(binding =  9, rgba8ui) uniform uimage2D instanceIndexImage;
layout(binding =  12, rgba8ui) uniform uimage2D resevSampleImage; // (x_z, m)
layout(binding =  13, rgba16f) uniform image2D resevWeightImage;  // (w_sum, p_z)
layout(binding =  3) buffer Addresses { BufferAddress address[]; } addresses;
layout(binding =  4) uniform sampler2D samplers[];

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
