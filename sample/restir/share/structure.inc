
#ifdef __cplusplus
#include <glm/glm.hpp>
#endif

struct Vertex
{
#ifdef __cplusplus
    glm::vec3 pos{0.0};
    glm::vec3 normal{0.0};
    glm::vec2 texCoord{0.0};
#else
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
#endif
};

struct SphereLight
{
#ifdef __cplusplus
    glm::vec3 intensity{1.0};
    glm::vec3 position{0.0};
    float radius{1.0};
#else
    vec3 intensity;
    vec3 position;
    float radius;
#endif
};

struct PointLight
{
#ifdef __cplusplus
    glm::vec3 intensity{1.0};
    glm::vec3 position{0.0};
#else
    vec3 intensity;
    vec3 position;
#endif
};

struct DirectionalLight
{
#ifdef __cplusplus
    glm::vec3 intensity{1.0};
    glm::vec3 direction{1.0};
#else
    vec3 intensity;
    vec3 direction;
#endif
};

struct ObjectData
{
#ifdef __cplusplus
    glm::mat4 matrix{1.0};
    glm::mat4 normalMatrix{1.0};
    glm::vec4 diffuse{1.0};
    glm::vec4 emission{0.0};
    glm::vec4 specular{0.0};
    int diffuseTexture{-1};
    int alphaTexture{-1};
#else
    mat4 matrix;
    mat4 normalMatrix;
    vec4 diffuse;
    vec4 emission;
    vec4 specular;
    int diffuseTexture;
    int alphaTexture;
#endif
};

struct BufferAddress
{
#ifdef __cplusplus
    using uint64_t = vk::DeviceAddress;
#endif
    uint64_t vertices;
    uint64_t indices;
    uint64_t objects;
    uint64_t pointLights;
    uint64_t sphereLights;
};

#ifdef __cplusplus
struct PushConstants
{
    glm::mat4 invView{ 1 };
    glm::mat4 invProj{ 1 };
    int frame = 0;
    int depth = 1;
    int samples = 1;
    int numLights = 0;
    glm::vec4 skyColor{ 0, 0, 0, 0 };
};
#else
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
#endif
