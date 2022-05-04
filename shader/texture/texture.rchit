#version 460
#extension GL_EXT_ray_tracing : enable

layout(binding = 4) uniform sampler2D samplers[];

layout(location = 0) rayPayloadInEXT vec3 payLoad;
hitAttributeEXT vec3 attribs;

void main()
{
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 color = texture(samplers[0], barycentricCoords.xy).rgb;

    payLoad = color;
}
