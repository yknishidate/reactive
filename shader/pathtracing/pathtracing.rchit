#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"
#include "random.glsl"

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

vec3 sampleSphereLight(in vec2 randVal, in vec3 dir)
{
    vec3 normal = -normalize(dir);
    vec3 tangent;
    vec3 bitangent;
    createCoordinateSystem(normal, tangent, bitangent);

    vec3 sampleDir = sampleHemisphere(randVal.x, randVal.y);
    return sampleDir.x * tangent + sampleDir.y * bitangent + sampleDir.z * normal;
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
    int diffuseTexture = objects.o[gl_InstanceID].diffuseTexture;
    if(diffuseTexture >= 0){
        vec4 value = texture(samplers[diffuseTexture], texCoord);
        diffuse = value.rgb;
        if(value.a < 1.0){
            payload.position = getWorldPos();
            payload.skip = true;
            return;
        }
    }

    mat4 matrix = objects.o[gl_InstanceID].matrix;
    mat4 normalMatrix = objects.o[gl_InstanceID].normalMatrix;
    pos = vec3(matrix * vec4(pos, 1));

    normal = normalize(vec3(normalMatrix * vec4(normal, 0)));
    vec3 brdf = diffuse / M_PI;

    // Next event estimation
    if(pushConstants.nee == 1) {
        SphereLight sphereLight = sphereLights.s[0];
        vec3 dir = sphereLight.position - pos;
        
        uvec2 s = pcg2d(ivec2(gl_LaunchIDEXT.xy) * (pushConstants.frame + 1));
        uint seed = s.x + s.y;
        vec3 point = sphereLight.position + sphereLight.radius * sampleSphereLight(vec2(rand(seed), rand(seed)), dir);
        dir = point - pos;

        float dist = sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);

        traceRayEXT(
            topLevelAS,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
            0xff, // cullMask
            0,    // sbtRecordOffset
            0,    // sbtRecordStride
            0,    // missIndex
            pos,            0.001,
            normalize(dir), dist,
            0     // payloadLocation
        );
        if(payload.done){
            float invDistPow2 = 1.0 / (dist * dist);
            float cosTheta = abs(dot(normalize(dir), normal));
            float pdf = 1.0 / (2.0 * M_PI * sphereLight.radius * sphereLight.radius);
            payload.color += payload.weight * sphereLight.intensity * brdf * invDistPow2 * cosTheta / pdf;
            payload.done = false;
        }
    }

    payload.emission = emission;
    payload.position = pos;
    payload.normal = normal;
    payload.brdf = brdf;
}
