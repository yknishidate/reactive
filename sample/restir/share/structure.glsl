
struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
};

struct SphereLight
{
    vec3 intensity;
    vec3 position;
    float radius;
};

struct PointLight
{
    vec3 intensity;
    vec3 position;
};

struct DirectionalLight
{
    vec3 intensity;
    vec3 direction;
};

struct ObjectData
{
    mat4 matrix;
    mat4 normalMatrix;
    vec4 diffuse;
    vec4 emission;
    vec4 specular;
    int diffuseTexture;
    int alphaTexture;
};

struct BufferAddress
{
    uint64_t vertices;
    uint64_t indices;
    uint64_t objects;
    uint64_t pointLights;
    uint64_t sphereLights;
};

layout(push_constant) uniform 
PushConstants{
    mat4 invView;
    mat4 invProj;
    int frame;
    int depth;
    int samples;
    int numLights;
    vec4 skyColor;
} pushConstants;
