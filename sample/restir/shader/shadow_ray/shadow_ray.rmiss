#version 460
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout(location = 0) rayPayloadInEXT ShadowPayload payload;

void main()
{
    payload.shadowed = false;
}