#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"
#include "random.glsl"
#define NUM_CANDIDATES 16

layout(location = 0) rayPayloadInEXT HitPayload payload;
hitAttributeEXT vec3 attribs;

vec3 getWorldPos()
{
    return gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
}

// TODO: rename
vec3 sampleSphereLight(in vec2 randVal, in vec3 dir)
{
    vec3 normal = -normalize(dir);
    vec3 tangent;
    vec3 bitangent;
    createCoordinateSystem(normal, tangent, bitangent);

    vec3 sampleDir = sampleHemisphere(randVal.x, randVal.y);
    return sampleDir.x * tangent + sampleDir.y * bitangent + sampleDir.z * normal;
}

int sampleLightUniform()
{
    if(pushConstants.numLights == 1) return 0;
    return int(rand(payload.seed) * float(pushConstants.numLights - 1.0));
}

// Target PDF: brdf * Le * G
float targetPDF(vec3 lightPos, vec3 intensity, vec3 pos, vec3 normal, vec3 brdf)
{
    vec3 dir = lightPos - pos;
    float invDistPow2 = 1.0 / (dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    float cosTheta = dot(normalize(dir), normal);
    return max(0.0, length(brdf) * length(intensity) * invDistPow2 * cosTheta);
}

void main()
{
    // Get buffer addresses
    BufferAddress address = addresses.address[gl_InstanceID];
    Vertices vertices = Vertices(address.vertices);
    Indices indices = Indices(address.indices);
    Objects objects = Objects(address.objects);
    PointLights pointLights = PointLights(address.pointLights);
    SphereLights sphereLights = SphereLights(address.sphereLights);
    
    // Get vertices
    uvec3 index = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[index.x];
    Vertex v1 = vertices.v[index.y];
    Vertex v2 = vertices.v[index.z];

    // Calc attributes
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos      = v0.pos      * barycentricCoords.x + v1.pos      * barycentricCoords.y + v2.pos      * barycentricCoords.z;
    vec3 normal   = v0.normal   * barycentricCoords.x + v1.normal   * barycentricCoords.y + v2.normal   * barycentricCoords.z;
    vec2 texCoord = v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y + v2.texCoord * barycentricCoords.z;

    // Get material
    vec3 diffuse = objects.o[gl_InstanceID].diffuse.rgb;
    vec3 emission = objects.o[gl_InstanceID].emission.rgb;

    // Sample diffuse texture
    int diffuseTexture = objects.o[gl_InstanceID].diffuseTexture;
    if(diffuseTexture >= 0){
        vec4 value = texture(samplers[diffuseTexture], texCoord);
        diffuse = value.rgb;

        // Skip transparence
        if(value.a < 1.0){
            payload.position = getWorldPos();
            payload.skip = true;
            return;
        }
    }
    vec3 brdf = diffuse / M_PI;

    // Get matrix
    mat4 matrix = objects.o[gl_InstanceID].matrix;
    mat4 normalMatrix = objects.o[gl_InstanceID].normalMatrix;
    pos = vec3(matrix * vec4(pos, 1));
    normal = normalize(vec3(normalMatrix * vec4(normal, 0)));

    // Sample light
    float pdf = 1.0;
    int lightIndex = -1;

    // Streaming RIS
    int y = 0;
    float sumWeights = 0.0;
    float storedTargetProb = 0.0;
    int numCandidates = 0;
    for(int i = 0; i < NUM_CANDIDATES; i++){
        // Sample candidates
        int candidate = sampleLightUniform();

        // Calc weights
        SphereLight sphereLight = sphereLights.s[candidate];
        vec3 lightPos = sphereLight.position;
        vec3 intensity = sphereLight.intensity;
        float targetProb = targetPDF(lightPos, intensity, pos, normal, brdf);
        float weight = targetProb;

        // Update
        sumWeights += weight;
        numCandidates += 1;
        if (rand(payload.seed) < (weight / sumWeights)) {
            y = candidate;
            storedTargetProb = targetProb;
        }
    }

    // if all zero, return
    if(sumWeights == 0.0){
        payload.emission = emission;
        payload.position = pos;
        payload.normal = normal;
        payload.brdf = brdf;
        return;
    }

    lightIndex = y;

    SphereLight sphereLight = sphereLights.s[lightIndex];

    // Sample point on light
    vec3 dir = sphereLight.position - pos;
    vec3 lightNormal = sampleSphereLight(vec2(rand(payload.seed), rand(payload.seed)), dir);
    vec3 point = sphereLight.position + sphereLight.radius * lightNormal;
    dir = point - pos;
    float dist = length(dir);
    
    // Trace rays
    traceRayEXT(
        topLevelAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xff, // cullMask
        0,    // sbtRecordOffset
        0,    // sbtRecordStride
        0,    // missIndex
        pos,            0.01,
        normalize(dir), dist,
        0     // payloadLocation
    );

    // If not shadowed, add contributes
    vec3 contribute = vec3(0.0);
    if(payload.done){
        float invDistPow2 = 1.0 / (dist * dist);
        float cosTheta = max(0.0, dot(normalize(dir), normal));
        float cosThetaLight = max(0.0, dot(normalize(-dir), lightNormal));
        pdf *= 1.0 / pushConstants.numLights;
        pdf *= 1.0 / (2.0 * M_PI * sphereLight.radius * sphereLight.radius);
        contribute = payload.weight * sphereLight.intensity * brdf * invDistPow2 * cosTheta * cosThetaLight / pdf;
        payload.done = false;
    }
    float reservoirWeight = (1.0 / storedTargetProb) * (sumWeights / numCandidates);
    payload.color += contribute * reservoirWeight;
    
    payload.emission = emission;
    payload.position = pos;
    payload.normal = normal;
    payload.brdf = brdf;
}
