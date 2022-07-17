// // Work dimensions
// in    uvec3  gl_LaunchIDEXT;
// in    uvec3  gl_LaunchSizeEXT;

// // Geometry instance ids
// in     int   gl_PrimitiveID;
// in     int   gl_InstanceID;
// in     int   gl_InstanceCustomIndexEXT;
// in     int   gl_GeometryIndexEXT;

// // World space parameters
// in    vec3   gl_WorldRayOriginEXT;
// in    vec3   gl_WorldRayDirectionEXT;
// in    vec3   gl_ObjectRayOriginEXT;
// in    vec3   gl_ObjectRayDirectionEXT;

// // Ray parameters
// in    float  gl_RayTminEXT;
// in    float  gl_RayTmaxEXT;
// in    uint   gl_IncomingRayFlagsEXT;

// // Ray hit info
// in    float  gl_HitTEXT;
// in    uint   gl_HitKindEXT;

// // Transform matrices
// in    mat4x3 gl_ObjectToWorldEXT;
// in    mat3x4 gl_ObjectToWorld3x4EXT;
// in    mat4x3 gl_WorldToObjectEXT;
// in    mat3x4 gl_WorldToObject3x4EXT;

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : enable

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
};

layout(location = 0) rayPayloadInEXT vec3 payload;
layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2) uniform sampler3D images;
layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
layout(push_constant) uniform PushConstants {
    mat4 invView;
    mat4 invProj;
    uint64_t vertices;
    uint64_t indices;
} constants;
hitAttributeEXT vec3 attribs;

int GRID_SIZE = 17;

float indexToDepth(int index)
{
    return (index + 0.5) / float(GRID_SIZE * GRID_SIZE);
}

int coordToIndex(vec2 coord)
{
    // coord: [0, 16]
    return int(GRID_SIZE * coord.y + coord.x);
}

float coordToDepth(vec2 coord)
{
    return indexToDepth(coordToIndex(coord));
}

bool traceRay(in vec3 origin, in vec3 direction)
{
    payload = vec3(0.0);
    traceRayEXT(
        topLevelAS,
        gl_RayFlagsOpaqueEXT,
        0xff, // cullMask
        0,    // sbtRecordOffset
        0,    // sbtRecordStride
        0,    // missIndex
        origin,
        0.001,
        direction,
        10000.0,
        0     // payloadLocation
    );
    return payload != vec3(0.0);
}

vec4 lightField(vec2 uv, vec2 st)
{
    // uv: camera
    // st: pixel
    // lt---rt
    // |     |
    // lb---rb

    // [0, 16]
    uv = uv * vec2(GRID_SIZE - 1, GRID_SIZE - 1);
    vec2 lt = floor(uv);
    vec2 rt = lt + vec2(1, 0);
    vec2 lb = lt + vec2(0, 1);
    vec2 rb = lt + vec2(1, 1);
    float depth_lt = coordToDepth(lt);
    float depth_rt = coordToDepth(rt);
    float depth_lb = coordToDepth(lb);
    float depth_rb = coordToDepth(rb);

    // linear interpolation in st plane is done at texture sampling time
    vec4 color_lt = texture(images, vec3(st, depth_lt));
    vec4 color_rt = texture(images, vec3(st, depth_rt));
    vec4 color_lb = texture(images, vec3(st, depth_lb));
    vec4 color_rb = texture(images, vec3(st, depth_rb));

    float a = uv.x - lt.x;
    float b = uv.y - lt.y;
    float weight_lt = (1.0 - a) * (1.0 - b);
    float weight_rt = a * (1.0 - b);
    float weight_lb = (1.0 - a) * b;
    float weight_rb = a * b;

    vec4 color = weight_lt * color_lt
               + weight_rt * color_rt
               + weight_lb * color_lb
               + weight_rb * color_rb;

    return color;
}

void main()
{
    // Get buffer addresses
    Vertices vertices = Vertices(constants.vertices);
    Indices indices = Indices(constants.indices);
    
    // Get vertices
    uvec3 index = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[index.x];
    Vertex v1 = vertices.v[index.y];
    Vertex v2 = vertices.v[index.z];

    // Calc attributes
    vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    vec2 texCoord = v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y + v2.texCoord * barycentricCoords.z;

    if(gl_InstanceID == 0){
        vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
        vec3 direction = gl_WorldRayDirectionEXT;

        if(traceRay(origin, direction)){
            // Hit
            vec2 st = payload.xy;
            vec2 uv = texCoord;
            vec4 color = lightField(uv, st);
            payload = color.rgb;
        }else{
            // Miss
            payload = vec3(0.2);
        }
    }
    if(gl_InstanceID == 1){
        payload = vec3(texCoord, 0);
    }
}
