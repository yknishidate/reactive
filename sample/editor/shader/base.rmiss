#version 460
#extension GL_EXT_ray_tracing : enable
#include "./share.h"

layout(location = 0) rayPayloadInEXT HitPayload payload;


vec2 sampleSphericalMap(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    return uv * invAtan + 0.5; // radian -> uv
}

void main()
{
    if(domeLightIndex != -1){
        InstanceDataBuffer dataBuffer = InstanceDataBuffer(instanceDataAddress);
        InstanceData data = dataBuffer.instanceData[domeLightIndex];
        if(data.baseColorTextureIndex == -1){
            payload.radiance = data.emissive;
        }else{
            vec2 uv = sampleSphericalMap(gl_WorldRayDirectionEXT.xyz);
            uv.x = mod(uv.x + radians(domeLightPhi) / (2 * PI), 1.0); // rotate phi
            uv.y = -uv.y;
            payload.radiance = texture(textures[data.baseColorTextureIndex], uv).rgb;
        }
        return;
    }

    payload.radiance = vec3(0.5);
}
