#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main()
{
    imageStore(posImage, ivec2(gl_LaunchIDEXT.xy), vec4(0));
    imageStore(normalImage, ivec2(gl_LaunchIDEXT.xy), vec4(0));
    imageStore(reservoirSampleImage, ivec2(gl_LaunchIDEXT.xy), uvec4(0));
    imageStore(reservoirWeightImage, ivec2(gl_LaunchIDEXT.xy), vec4(0));
    imageStore(indexImage, ivec2(gl_LaunchIDEXT.xy), uvec4(0));
    payload.done = true;
}
