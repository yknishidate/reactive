#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : enable

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

struct MeshAddress
{
    uint64_t vertices;
    uint64_t indices;
    uint64_t objects;
    uint64_t pointLights;
    uint64_t sphereLights;
};

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
};

struct ObjectData
{
    mat4 matrix;
    mat4 normalMatrix;
    vec4 diffuse;
    vec4 emission;
    vec4 specular;
    int diffuseTexture;
    int alphaTexture;
};

struct PointLight
{
    vec3 intensity;
    vec3 position;
};

struct SphereLight
{
    vec3 intensity;
    vec3 position;
    float radius;
};

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
layout(buffer_reference, scalar) buffer Objects { ObjectData o[]; };
layout(buffer_reference, scalar) buffer PointLights { PointLight p[]; };
layout(buffer_reference, scalar) buffer SphereLights { SphereLight s[]; };
layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D inputImage;
layout(binding = 2, set = 0, rgba8) uniform image2D outputImage;
layout(binding = 3) buffer Addresses { MeshAddress address[]; } addresses;
layout(binding = 4) uniform sampler2D samplers[];

layout(push_constant) uniform PushConstants{
    mat4 invView;
    mat4 invProj;
    int frame;
    int importance;
    int depth;
    int samples;
    vec4 skyColor;
    int nee;
    int numLights;
} pushConstants;

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
