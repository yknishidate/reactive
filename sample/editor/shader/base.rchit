#version 460
#extension GL_EXT_ray_tracing : enable
#include "./share.h"

layout(location = 0) rayPayloadInEXT HitPayload payload;

hitAttributeEXT vec3 attribs;

void main()
{
    InstanceDataBuffer dataBuffer = InstanceDataBuffer(instanceDataAddress);
    InstanceData data = dataBuffer.instanceData[gl_InstanceID];
    payload.radiance = data.baseColor.rgb;
}
