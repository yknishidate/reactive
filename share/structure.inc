
#ifdef __cplusplus
    using vec2 = glm::vec2;
    using vec3 = glm::vec3;
    using vec4 = glm::vec4;
    using mat4 = glm::mat4;
#endif

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
