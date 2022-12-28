#include "Engine.hpp"
#include "Scene/Loader.hpp"

struct SphereLight {
    glm::vec3 intensity{1.0};
    glm::vec3 position{0.0};
    float radius{1.0};
};

struct PushConstants {
    glm::mat4 invView{1};
    glm::mat4 invProj{1};
    int frame = 0;
    int depth = 1;
    int samples = 1;
    int numLights = 0;
    glm::vec4 skyColor{0, 0, 0, 0};
};

struct OutputImage {
    Image output{vk::Format::eB8G8R8A8Unorm};
};

struct GBuffers {
    Image position{vk::Format::eR16G16B16A16Sfloat};
    Image normal{vk::Format::eR16G16B16A16Sfloat};
    Image diffuse{vk::Format::eB8G8R8A8Unorm};
    Image emission{vk::Format::eR16G16B16A16Sfloat};
    Image instanceIndex{vk::Format::eR8G8B8A8Uint};
};

struct ResevImages {
    void copy(vk::CommandBuffer commandBuffer, ResevImages& dst) {
        sample.setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
        weight.setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
        dst.sample.setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        dst.weight.setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        sample.copyToImage(commandBuffer, dst.sample);
        weight.copyToImage(commandBuffer, dst.weight);
        sample.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        weight.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        dst.sample.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        dst.weight.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
    }

    Image sample{vk::Format::eR32G32Uint};
    Image weight{vk::Format::eR16G16Sfloat};
};

struct BufferAddress {
    vk::DeviceAddress vertices;
    vk::DeviceAddress indices;
    vk::DeviceAddress objects;
    vk::DeviceAddress pointLights;
    vk::DeviceAddress sphereLights;
};

struct BoundingBox {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{-std::numeric_limits<float>::max()};
};

class Scene {
public:
    Scene() = default;
    explicit Scene(const std::string& filepath,
                   glm::vec3 position = glm::vec3{0.0f},
                   glm::vec3 scale = glm::vec3{1.0f},
                   glm::vec3 rotation = glm::vec3{0.0f}) {
        Loader::loadFromFile(ASSET_DIR + filepath, meshes, textures);

        // If textures is empty, append dummy texture
        if (textures.empty()) {
            textures = std::vector<Image>(1);
            textures[0] = Image{1, 1, vk::Format::eR8G8B8A8Unorm};
        }

        // Create objects
        objects.reserve(meshes.size());
        for (int i = 0; i < meshes.size(); i++) {
            Object object(meshes[i]);
            object.transform.position = position;
            object.transform.scale = scale;
            object.transform.rotation = glm::quat{rotation};
            for (const auto& vertex : meshes[i].getVertices()) {
                glm::vec3 pos = object.transform.getMatrix() * glm::vec4{vertex.pos, 1.0};
                bbox.min = glm::min(bbox.min, pos);
                bbox.max = glm::max(bbox.max, pos);
            }
            objects.push_back(object);
        }
    }

    void setup() {
        topAccel = TopAccel{objects};

        // Create object data
        for (auto& object : objects) {
            ObjectData data;
            data.matrix = object.transform.getMatrix();
            data.normalMatrix = object.transform.getNormalMatrix();
            data.diffuse = glm::vec4{object.getMaterial().diffuse, 1};
            data.emission = glm::vec4{object.getMaterial().emission, 1};
            data.specular = glm::vec4{0.0f};
            data.diffuseTexture = object.getMaterial().diffuseTexture;
            data.alphaTexture = object.getMaterial().alphaTexture;
            objectData.push_back(data);
        }
        objectBuffer = DeviceBuffer{BufferUsage::Storage, objectData};
        if (!sphereLights.empty()) {
            sphereLightBuffer = DeviceBuffer{BufferUsage::Storage, sphereLights};
        }

        // Buffer references
        std::vector<BufferAddress> addresses(objects.size());
        for (int i = 0; i < objects.size(); i++) {
            addresses[i].vertices = objects[i].getMesh().getVertexBufferAddress();
            addresses[i].indices = objects[i].getMesh().getIndexBufferAddress();
            addresses[i].objects = objectBuffer.getAddress();
            if (!sphereLights.empty()) {
                addresses[i].sphereLights = sphereLightBuffer.getAddress();
            }
        }
        addressBuffer = DeviceBuffer{BufferUsage::Storage, addresses};

        camera = Camera{Window::getWidth(), Window::getHeight()};
    }

    void update(float dt) { camera.processInput(); }

    Mesh& addMesh(const std::string& filepath) { return meshes.emplace_back(filepath); }

    SphereLight& addSphereLight(glm::vec3 intensity, glm::vec3 position, float radius) {
        static bool added = false;
        static int sphereMeshIndex;
        if (!added) {
            Material lightMaterial;
            lightMaterial.emission = glm::vec3{1.0f};

            Mesh& mesh = addMesh("Sphere.obj");
            mesh.setMaterial(lightMaterial);
            sphereMeshIndex = meshes.size() - 1;
        }
        added = true;

        Material lightMaterial;
        lightMaterial.emission = intensity;

        auto& obj = objects.emplace_back(meshes[sphereMeshIndex]);
        obj.transform.position = position;
        obj.transform.scale = glm::vec3{radius};
        obj.setMaterial(lightMaterial);

        return sphereLights.emplace_back(intensity, position, radius);
    }

    const auto& getAccel() const { return topAccel; }
    const auto& getTextures() const { return textures; }
    const auto& getAddressBuffer() const { return addressBuffer; }
    auto& getCamera() { return camera; }
    auto& getObjects() { return objects; }
    auto getBoundingBox() const { return bbox; }
    auto getCenter() const { return (bbox.min + bbox.max) / 2.0f; }
    auto getNumSphereLights() const { return sphereLights.size(); }

private:
    std::vector<Mesh> meshes;
    std::vector<Image> textures;

    std::vector<Object> objects;
    std::vector<ObjectData> objectData;

    BoundingBox bbox;

    TopAccel topAccel;
    Camera camera;

    DeviceBuffer objectBuffer;
    DeviceBuffer addressBuffer;

    std::vector<SphereLight> sphereLights;
    DeviceBuffer sphereLightBuffer;
};

int main() {
    try {
        Log::init();
        Window::init(1200, 1000);
        Context::init();

        Swapchain swapchain{};
        GUI gui{swapchain};

        Scene scene{"crytek_sponza/sponza.obj", glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.01f},
                    glm::vec3{0.0f, glm::radians(90.0f), 0.0f}};

        BoundingBox bbox = scene.getBoundingBox();
        std::mt19937 mt{};
        std::uniform_real_distribution distX{bbox.min.x, bbox.max.x};
        std::uniform_real_distribution distY{bbox.min.y, bbox.max.y};
        std::uniform_real_distribution distZ{bbox.min.z, bbox.max.z};
        for (int index = 0; index < 100; index++) {
            const glm::vec3 position = glm::vec3{distX(mt), distY(mt), distZ(mt)} / 2.5f;
            const glm::vec3 color{1.0f};
            scene.addSphereLight(color, position, 0.1f);
        }
        scene.setup();

        GBuffers gbuffers;
        OutputImage outputImage;
        ResevImages initedResevImages;
        ResevImages reusedResevImages;
        ResevImages storedResevImages;

        Shader gbufferRgen{SHADER_DIR + "gbuffer/gbuffer.rgen"};
        Shader gbufferMiss{SHADER_DIR + "gbuffer/gbuffer.rmiss"};
        Shader gbufferChit{SHADER_DIR + "gbuffer/gbuffer.rchit"};
        Shader shadowRayMiss{SHADER_DIR + "shadow_ray/shadow_ray.rmiss"};
        Shader shadowRayChit{SHADER_DIR + "shadow_ray/shadow_ray.rchit"};
        Shader uniformRgen{SHADER_DIR + "uniform_light/uniform_light.rgen"};
        Shader wrsRgen{SHADER_DIR + "wrs/wrs.rgen"};
        Shader initResevRgen{SHADER_DIR + "restir/init_resev.rgen"};
        Shader spatialReuseRgen{SHADER_DIR + "restir/spatial_reuse.rgen"};
        Shader tempReuseRgen{SHADER_DIR + "restir/temporal_reuse.rgen"};
        Shader shadingRgen{SHADER_DIR + "restir/shading.rgen"};

        DescriptorSet descSet{};
        descSet.addResources(gbufferRgen);
        descSet.addResources(gbufferMiss);
        descSet.addResources(gbufferChit);
        descSet.addResources(shadowRayMiss);
        descSet.addResources(shadowRayChit);
        descSet.addResources(uniformRgen);
        descSet.addResources(wrsRgen);
        descSet.addResources(initResevRgen);
        descSet.addResources(spatialReuseRgen);
        descSet.addResources(tempReuseRgen);
        descSet.addResources(shadingRgen);
        descSet.record("topLevelAS", scene.getAccel());
        descSet.record("samplers", scene.getTextures());
        descSet.record("Addresses", scene.getAddressBuffer());
        descSet.record("positionImage", gbuffers.position);
        descSet.record("normalImage", gbuffers.normal);
        descSet.record("diffuseImage", gbuffers.diffuse);
        descSet.record("emissionImage", gbuffers.emission);
        descSet.record("instanceIndexImage", gbuffers.instanceIndex);
        descSet.record("outputImage", outputImage.output);
        descSet.record("resevSampleImage", initedResevImages.sample);
        descSet.record("resevWeightImage", initedResevImages.weight);
        descSet.record("newResevSampleImage", reusedResevImages.sample);
        descSet.record("newResevWeightImage", reusedResevImages.weight);
        descSet.record("oldResevSampleImage", storedResevImages.sample);
        descSet.record("oldResevWeightImage", storedResevImages.weight);
        descSet.allocate();

        std::unordered_map<std::string, RayTracingPipeline> pipelines;
        pipelines.insert({"GBuffer", RayTracingPipeline{descSet}});
        pipelines.insert({"Uniform", RayTracingPipeline{descSet}});
        pipelines.insert({"WRS", RayTracingPipeline{descSet}});
        pipelines.insert({"InitResev", RayTracingPipeline{descSet}});
        pipelines.insert({"SpatialReuse", RayTracingPipeline{descSet}});
        pipelines.insert({"TemporalReuse", RayTracingPipeline{descSet}});
        pipelines.insert({"Shading", RayTracingPipeline{descSet}});

        pipelines["GBuffer"].setShaders(gbufferRgen, gbufferMiss, gbufferChit);
        pipelines["Uniform"].setShaders(uniformRgen, shadowRayMiss, shadowRayChit);
        pipelines["WRS"].setShaders(wrsRgen, shadowRayMiss, shadowRayChit);
        pipelines["InitResev"].setShaders(initResevRgen, shadowRayMiss, shadowRayChit);
        pipelines["SpatialReuse"].setShaders(spatialReuseRgen, shadowRayMiss, shadowRayChit);
        pipelines["TemporalReuse"].setShaders(tempReuseRgen, shadowRayMiss, shadowRayChit);
        pipelines["Shading"].setShaders(shadingRgen, shadowRayMiss, shadowRayChit);
        for (auto& [name, pipeline] : pipelines) {
            pipeline.setup(sizeof(PushConstants));
        }

        PushConstants pushConstants;
        pushConstants.numLights = scene.getNumSphereLights();
        pushConstants.invProj = scene.getCamera().getInvProj();
        pushConstants.invView = scene.getCamera().getInvView();
        pushConstants.frame = 0;

        int method = 0;
        int iteration = 0;
        bool temporalReuse = false;
        constexpr int Uniform = 0;
        constexpr int WRS = 1;
        constexpr int ReSTIR = 2;
        while (!Window::shouldClose()) {
            Window::pollEvents();

            // Add GUI
            gui.startFrame();
            gui.combo("Method", method, {"Uniform", "WRS", "ReSTIR"});
            gui.sliderInt("Samples", pushConstants.samples, 1, 32);
            if (method == ReSTIR) {
                gui.sliderInt("Iteration", iteration, 0, 4);
                gui.checkbox("Temporal reuse", temporalReuse);
            }

            // Update scene and push constants
            scene.update(0.1);
            pushConstants.invProj = scene.getCamera().getInvProj();
            pushConstants.invView = scene.getCamera().getInvView();
            pushConstants.frame++;

            swapchain.waitNextFrame();

            // Record commands
            auto width = Window::getWidth();
            auto height = Window::getHeight();
            CommandBuffer commandBuffer = swapchain.beginCommandBuffer();
            commandBuffer.bindPipeline(pipelines["GBuffer"]);
            commandBuffer.pushConstants(pipelines["GBuffer"], &pushConstants);
            commandBuffer.traceRays(pipelines["GBuffer"], width, height);

            if (method == Uniform) {
                commandBuffer.bindPipeline(pipelines["Uniform"]);
                commandBuffer.pushConstants(pipelines["Uniform"], &pushConstants);
                commandBuffer.traceRays(pipelines["Uniform"], width, height);
            } else if (method == WRS) {
                commandBuffer.bindPipeline(pipelines["WRS"]);
                commandBuffer.pushConstants(pipelines["WRS"], &pushConstants);
                commandBuffer.traceRays(pipelines["WRS"], width, height);
            } else if (method == ReSTIR) {
                commandBuffer.bindPipeline(pipelines["InitResev"]);
                commandBuffer.pushConstants(pipelines["InitResev"], &pushConstants);
                commandBuffer.traceRays(pipelines["InitResev"], width, height);

                if (temporalReuse) {
                    commandBuffer.bindPipeline(pipelines["TemporalReuse"]);
                    commandBuffer.pushConstants(pipelines["TemporalReuse"], &pushConstants);
                    commandBuffer.traceRays(pipelines["TemporalReuse"], width, height);
                    reusedResevImages.copy(commandBuffer.commandBuffer, initedResevImages);
                }
                initedResevImages.copy(commandBuffer.commandBuffer, storedResevImages);
                for (int i = 0; i < iteration; i++) {
                    commandBuffer.bindPipeline(pipelines["SpatialReuse"]);
                    commandBuffer.pushConstants(pipelines["SpatialReuse"], &pushConstants);
                    commandBuffer.traceRays(pipelines["SpatialReuse"], width, height);
                    reusedResevImages.copy(commandBuffer.commandBuffer, initedResevImages);
                }
                commandBuffer.bindPipeline(pipelines["Shading"]);
                commandBuffer.pushConstants(pipelines["Shading"], &pushConstants);
                commandBuffer.traceRays(pipelines["Shading"], width, height);
            }

            commandBuffer.copyToBackImage(outputImage.output);

            // Draw GUI
            commandBuffer.beginRenderPass();
            commandBuffer.drawGUI(gui);
            commandBuffer.endRenderPass();

            commandBuffer.submit();

            swapchain.present();
        }
        Context::waitIdle();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
