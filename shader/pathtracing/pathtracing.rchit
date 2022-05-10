#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;
hitAttributeEXT vec3 attribs;

vec3 getWorldPos()
{
    return gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 getInstanceColor()
{
    float hue = gl_InstanceID % 25 / 25.0;
    return hsv2rgb(vec3(hue, 0.7, 1.0));
}

void main()
{
    MeshAddress address = addresses.address[gl_InstanceID];
    Vertices vertices = Vertices(address.vertices);
    Indices indices = Indices(address.indices);
    Objects objects = Objects(address.objects);
    PointLights pointLights = PointLights(address.pointLights);
    SphereLights sphereLights = SphereLights(address.sphereLights);

    uvec3 index = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[index.x];
    Vertex v1 = vertices.v[index.y];
    Vertex v2 = vertices.v[index.z];

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos      = v0.pos      * barycentricCoords.x + v1.pos      * barycentricCoords.y + v2.pos      * barycentricCoords.z;
    vec3 normal   = v0.normal   * barycentricCoords.x + v1.normal   * barycentricCoords.y + v2.normal   * barycentricCoords.z;
    vec2 texCoord = v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y + v2.texCoord * barycentricCoords.z;

    vec3 diffuse = objects.o[gl_InstanceID].diffuse.rgb;
    vec3 emission = objects.o[gl_InstanceID].emission.rgb;
    int textureIndex = objects.o[gl_InstanceID].textureIndex;
    if(textureIndex >= 0){
        diffuse = texture(samplers[textureIndex], texCoord).rgb;
    }

    mat4 matrix = objects.o[gl_InstanceID].matrix;
    mat4 normalMatrix = objects.o[gl_InstanceID].normalMatrix;
    pos = vec3(matrix * vec4(pos, 1));
    normal = normalize(vec3(normalMatrix * vec4(normal, 0)));

    vec3 brdf = diffuse / M_PI;

//    // Next event estimation
//    if(pushConstants.nee == 1) {
//        PointLight pointLight = pointLights.p[0];
//        vec3 dir = pointLight.position - pos;
//        float dist = sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
//
//        traceRayEXT(
//            topLevelAS,
//            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
//            0xff, // cullMask
//            0,    // sbtRecordOffset
//            0,    // sbtRecordStride
//            0,    // missIndex
//            pos + normal * 0.001,
//            0.001,
//            normalize(dir),
//            dist,
//            0     // payloadLocation
//        );
//        if(payload.done){
//            float invDistPow2 = 1.0 / dist * dist;
//            float cosTheta = dot(normalize(dir), normal);
//            float pdf = 1.0;
////            payload.color += payload.weight * pointLight.intensity * brdf * invDistPow2 * cosTheta / pdf;
//            payload.done = false;
//        }
//    }

//  for debug
//    payload.done = true;
    payload.emission = emission;
    payload.position = pos;
    payload.normal = normal;
    payload.brdf = brdf;
}
