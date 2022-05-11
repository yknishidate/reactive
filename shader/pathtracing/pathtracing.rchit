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
    return int(rand(payload.seed) * float(pushConstants.numLights - 1.0));
}

// Target PDF: brdf * Le * G
float targetPDF(vec3 lightPos, vec3 intensity, vec3 pos, vec3 normal, vec3 brdf)
{
    vec3 dir = lightPos - pos;
    float invDistPow2 = 1.0 / length(dir);
    float cosTheta = dot(normalize(dir), normal);
    return max(0.0, length(brdf) * length(intensity) * invDistPow2 * cosTheta);
}

void main()
{
    // Get buffer addresses
    MeshAddress address = addresses.address[gl_InstanceID];
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

    if(pushConstants.nee == 0){
        payload.emission = emission;
        payload.position = pos;
        payload.normal = normal;
        payload.brdf = brdf;
        return;
    }

    // Sample light
    float pdf = 1.0;
    int lightIndex = -1;
    if(pushConstants.nee == 1){
        lightIndex = sampleLightUniform();
    } else if(pushConstants.nee == 2) {
        int candidates[32];
        float weights[32];
        float sumWeights = 0.0;
        for(int i = 0; i < 32; i++){
            // Sample candidates
            int candidate = sampleLightUniform();
            candidates[i] = candidate;
        
            // Calc weights
            SphereLight sphereLight = sphereLights.s[candidate];
            vec3 lightPos = sphereLight.position;
            vec3 intensity = sphereLight.intensity;
            float tgtPDF = targetPDF(lightPos, intensity, pos, normal, brdf);
            float weight = tgtPDF; // srcPDF = 1.0
            
            weights[i] = weight;
            sumWeights += weight;
        }
        
        // Sample using weights
        float cumulation = 0.0;
        float threshold = rand(payload.seed) * sumWeights;
        float tgtPDF;
        for(int i = 0; i < 32; i++){
            cumulation += weights[i];
            if (threshold <= cumulation) {
                lightIndex = candidates[i];
                tgtPDF = weights[i]; // WARN: pdf != weight
                break;
            }
        }
        pdf = tgtPDF / sumWeights * 32.0;
    } 
    SphereLight sphereLight = sphereLights.s[lightIndex];

    // Sample point on light
    vec3 dir = sphereLight.position - pos;
    float dist = length(dir) - sphereLight.radius;
    
    // Trace rays
    //traceRayEXT(
    //    topLevelAS,
    //    gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
    //    0xff, // cullMask
    //    0,    // sbtRecordOffset
    //    0,    // sbtRecordStride
    //    0,    // missIndex
    //    pos,            0.01,
    //    normalize(dir), dist,
    //    0     // payloadLocation
    //);

    // If not shadowed, add contributes
    //if(payload.done){
        float invDistPow2 = 1.0 / (dist * dist);
        float cosTheta = abs(dot(normalize(dir), normal));
        //pdf *= 1.0 / (4.0 * M_PI * (sphereLight.radius * sphereLight.radius));
        payload.color += payload.weight * sphereLight.intensity * brdf * invDistPow2 * cosTheta / pdf;
        payload.done = false;
    //}

    payload.emission = emission;
    payload.position = pos;
    payload.normal = normal;
    payload.brdf = brdf;
}
