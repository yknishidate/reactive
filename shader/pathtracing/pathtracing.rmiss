#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payLoad;

void main()
{
    payLoad.emittion = vec3(1.0);
    payLoad.done = true;
}
