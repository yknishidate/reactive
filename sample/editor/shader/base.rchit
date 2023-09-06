#version 460
#extension GL_EXT_ray_tracing : enable
#include "./share.h"
#include "./random.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

hitAttributeEXT vec3 attribs;

void traceRay(vec3 origin, vec3 direction) {
    traceRayEXT(
        topLevelAS,
        gl_RayFlagsOpaqueEXT,
        0xff, // cullMask
        0,    // sbtRecordOffset
        0,    // sbtRecordStride
        0,    // missIndex
        origin,
        0.001,
        direction,
        1000.0,
        0     // payloadLocation
    );
}

void main()
{
    InstanceDataBuffer dataBuffer = InstanceDataBuffer(instanceDataAddress);
    InstanceData data = dataBuffer.instanceData[gl_InstanceID];

    VertexBuffer vertexBuffer = VertexBuffer(data.vertexAddress);
    IndexBuffer indexBuffer = IndexBuffer(data.indexAddress);

    uint i0 = indexBuffer.indices[3 * gl_PrimitiveID + 0];
    uint i1 = indexBuffer.indices[3 * gl_PrimitiveID + 1];
    uint i2 = indexBuffer.indices[3 * gl_PrimitiveID + 2];

    Vertex v0 = vertexBuffer.vertices[i0];
    Vertex v1 = vertexBuffer.vertices[i1];
    Vertex v2 = vertexBuffer.vertices[i2];

    vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);
    vec2 texCoord = v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y + v2.texCoord * barycentricCoords.z;
    
    normal = normalize(mat3(data.normalMatrix) * normal);

    // Get material
    vec3 baseColor = data.baseColor.rgb;
    float transmission = 1.0 - data.baseColor.a;
    float metallic = data.metallic;
    float roughness = data.roughness;
    vec3 emissive = data.emissive;

    payload.depth += 1;
    if(payload.depth >= 24){
        payload.radiance = emissive;
        return;
    }

    vec3 origin = pos;
    vec3 direction = reflect(gl_WorldRayDirectionEXT, normal);
    traceRay(origin, direction);

    payload.radiance = emissive + baseColor * payload.radiance;
}
