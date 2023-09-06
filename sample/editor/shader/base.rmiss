#version 460
#extension GL_EXT_ray_tracing : enable
#include "./share.h"

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main()
{
    if(domeLightIndex != -1){
        InstanceDataBuffer dataBuffer = InstanceDataBuffer(instanceDataAddress);
        InstanceData data = dataBuffer.instanceData[domeLightIndex];
        payload.radiance = data.emissive;
        return;
    }

    payload.radiance = vec3(0.5);
}
