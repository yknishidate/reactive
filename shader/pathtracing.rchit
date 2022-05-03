#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : enable

struct HitPayload
{
    vec3 position;
    vec3 normal;
    vec3 emittion;
    vec3 brdf;
    bool done;
};

struct MeshAddress
{
    uint64_t vertices;
    uint64_t indices;
};

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
};

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
layout(binding = 3) buffer Addresses { MeshAddress address[]; } addresses;
layout(binding = 4) uniform sampler2D samplers[];

layout(location = 0) rayPayloadInEXT HitPayload payLoad;
hitAttributeEXT vec3 attribs;

const highp float M_PI = 3.14159265358979323846;

void main()
{
    MeshAddress address = addresses.address[0];
    Vertices vertices = Vertices(address.vertices);
    Indices indices = Indices(address.indices);

    uvec3 index = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[index.x];
    Vertex v1 = vertices.v[index.y];
    Vertex v2 = vertices.v[index.z];

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos      = v0.pos      * barycentricCoords.x + v1.pos      * barycentricCoords.y + v2.pos      * barycentricCoords.z;
    vec3 normal   = v0.normal   * barycentricCoords.x + v1.normal   * barycentricCoords.y + v2.normal   * barycentricCoords.z;
    vec2 texCoord = v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y + v2.texCoord * barycentricCoords.z;
    vec3 color = texture(samplers[0], texCoord).rgb;

    payLoad.position = pos;
    payLoad.normal = normal;
    payLoad.brdf = color / M_PI;
}
