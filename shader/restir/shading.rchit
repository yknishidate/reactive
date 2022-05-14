#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"
#include "random.glsl"
#define NUM_CANDIDATES 16

layout(location = 0) rayPayloadInEXT HitPayload payload;
hitAttributeEXT vec3 attribs;

void main()
{
}
