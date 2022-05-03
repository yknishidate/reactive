#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 payLoad;
hitAttributeEXT vec3 attribs;

void main()
{
    payLoad = vec3(1.0);
}
