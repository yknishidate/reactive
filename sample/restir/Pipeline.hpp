#pragma once
#include "Vulkan/Pipeline.hpp"
#include "Window/Window.hpp"

struct GBuffers
{
    Image position;
    Image normal;
    Image diffuse;
    Image emission;
    Image instanceIndex;
};

class GBufferPipeline : public RayTracingPipeline
{
public:
    GBufferPipeline(const Scene& scene, size_t pushSize = 0)
    {
        descSet = std::make_shared<DescriptorSet>();
        RayTracingPipeline::LoadShaders(SHADER_DIR + "gbuffer/gbuffer.rgen",
                                        SHADER_DIR + "gbuffer/gbuffer.rmiss",
                                        SHADER_DIR + "gbuffer/gbuffer.rchit");
        Register("topLevelAS", scene.GetAccel());
        Register("samplers", scene.GetTextures());
        Register("Addresses", scene.GetAddressBuffer());

        gbuffers.position = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR16G16B16A16Sfloat };
        gbuffers.normal = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR16G16B16A16Sfloat };
        gbuffers.diffuse = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm };
        gbuffers.emission = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR16G16B16A16Sfloat };
        gbuffers.instanceIndex = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR8G8B8A8Uint };

        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);

        RayTracingPipeline::Setup(pushSize);
    }

    const GBuffers& GetGBuffers() const { return gbuffers; }

private:
    GBuffers gbuffers;
};

class UniformLightPipeline : public RayTracingPipeline
{
public:
    UniformLightPipeline(const Scene& scene, const GBuffers& gbuffers, size_t pushSize = 0)
    {
        descSet = std::make_shared<DescriptorSet>();
        RayTracingPipeline::LoadShaders(SHADER_DIR + "uniform_light/uniform_light.rgen",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rchit");
        Register("topLevelAS", scene.GetAccel());
        Register("samplers", scene.GetTextures());
        Register("Addresses", scene.GetAddressBuffer());

        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);

        outputImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm };
        Register("outputImage", outputImage);

        RayTracingPipeline::Setup(pushSize);
    }

    const Image& GetOutputImage() const { return outputImage; }

private:
    Image outputImage;
};

class WRSPipeline : public RayTracingPipeline
{
public:
    WRSPipeline(const Scene& scene, const GBuffers& gbuffers, size_t pushSize = 0)
    {
        descSet = std::make_shared<DescriptorSet>();
        RayTracingPipeline::LoadShaders(SHADER_DIR + "wrs/wrs.rgen",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rchit");
        Register("topLevelAS", scene.GetAccel());
        Register("samplers", scene.GetTextures());
        Register("Addresses", scene.GetAddressBuffer());

        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);

        outputImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm };
        Register("outputImage", outputImage);
        RayTracingPipeline::Setup(pushSize);
    }

    const Image& GetOutputImage() const { return outputImage; }

private:
    Image outputImage;
};

struct ResevImages
{
    Image sampleImage;
    Image weightImage;
};

class InitResevPipeline : public RayTracingPipeline
{
public:
    InitResevPipeline(const Scene& scene, const GBuffers& gbuffers, size_t pushSize = 0)
    {
        descSet = std::make_shared<DescriptorSet>();
        RayTracingPipeline::LoadShaders(SHADER_DIR + "restir/init_resev.rgen",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rchit");
        Register("topLevelAS", scene.GetAccel());
        Register("samplers", scene.GetTextures());
        Register("Addresses", scene.GetAddressBuffer());

        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);

        resevImages.sampleImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR32G32Uint };
        resevImages.weightImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR16G16Sfloat };
        Register("resevSampleImage", resevImages.sampleImage);
        Register("resevWeightImage", resevImages.weightImage);

        RayTracingPipeline::Setup(pushSize);
    }

    ResevImages& GetResevImages() { return resevImages; }

private:
    ResevImages resevImages;
};

class SpatialReusePipeline : public RayTracingPipeline
{
public:
    SpatialReusePipeline(const Scene& scene, const GBuffers& gbuffers, const ResevImages& resevImages, size_t pushSize = 0)
    {
        descSet = std::make_shared<DescriptorSet>();
        RayTracingPipeline::LoadShaders(SHADER_DIR + "restir/spatial_reuse.rgen",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rchit");
        Register("topLevelAS", scene.GetAccel());
        Register("samplers", scene.GetTextures());
        Register("Addresses", scene.GetAddressBuffer());

        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);
        Register("resevSampleImage", resevImages.sampleImage);
        Register("resevWeightImage", resevImages.weightImage);

        newResevImages.sampleImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR32G32Uint };
        newResevImages.weightImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR16G16Sfloat };
        Register("newResevSampleImage", newResevImages.sampleImage);
        Register("newResevWeightImage", newResevImages.weightImage);

        RayTracingPipeline::Setup(pushSize);
    }

    ResevImages& GetNewResevImages() { return newResevImages; }

    void CopyToResevImages(ResevImages& dst)
    {
        newResevImages.sampleImage.SetImageLayout(vk::ImageLayout::eTransferSrcOptimal);
        newResevImages.weightImage.SetImageLayout(vk::ImageLayout::eTransferSrcOptimal);
        dst.sampleImage.SetImageLayout(vk::ImageLayout::eTransferDstOptimal);
        dst.weightImage.SetImageLayout(vk::ImageLayout::eTransferDstOptimal);

        newResevImages.sampleImage.CopyToImage(dst.sampleImage);
        newResevImages.weightImage.CopyToImage(dst.weightImage);

        newResevImages.sampleImage.SetImageLayout(vk::ImageLayout::eGeneral);
        newResevImages.weightImage.SetImageLayout(vk::ImageLayout::eGeneral);
        dst.sampleImage.SetImageLayout(vk::ImageLayout::eGeneral);
        dst.weightImage.SetImageLayout(vk::ImageLayout::eGeneral);
    }

private:
    ResevImages newResevImages;
};

class TemporalReusePipeline : public RayTracingPipeline
{
public:
    TemporalReusePipeline(const Scene& scene, const GBuffers& gbuffers, const ResevImages& resevImages, size_t pushSize = 0)
    {
        descSet = std::make_shared<DescriptorSet>();
        RayTracingPipeline::LoadShaders(SHADER_DIR + "restir/temporal_reuse.rgen",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rchit");
        Register("topLevelAS", scene.GetAccel());
        Register("samplers", scene.GetTextures());
        Register("Addresses", scene.GetAddressBuffer());

        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);
        Register("resevSampleImage", resevImages.sampleImage);
        Register("resevWeightImage", resevImages.weightImage);

        newResevImages.sampleImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR32G32Uint };
        newResevImages.weightImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR16G16Sfloat };
        oldResevImages.sampleImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR32G32Uint };
        oldResevImages.weightImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR16G16Sfloat };
        Register("newResevSampleImage", newResevImages.sampleImage);
        Register("newResevWeightImage", newResevImages.weightImage);
        Register("oldResevSampleImage", oldResevImages.sampleImage);
        Register("oldResevWeightImage", oldResevImages.weightImage);

        RayTracingPipeline::Setup(pushSize);
    }

    ResevImages& GetNewResevImages() { return newResevImages; }

    void CopyToResevImages(ResevImages& dst)
    {
        newResevImages.sampleImage.SetImageLayout(vk::ImageLayout::eTransferSrcOptimal);
        newResevImages.weightImage.SetImageLayout(vk::ImageLayout::eTransferSrcOptimal);
        dst.sampleImage.SetImageLayout(vk::ImageLayout::eTransferDstOptimal);
        dst.weightImage.SetImageLayout(vk::ImageLayout::eTransferDstOptimal);

        newResevImages.sampleImage.CopyToImage(dst.sampleImage);
        newResevImages.weightImage.CopyToImage(dst.weightImage);

        newResevImages.sampleImage.SetImageLayout(vk::ImageLayout::eGeneral);
        newResevImages.weightImage.SetImageLayout(vk::ImageLayout::eGeneral);
        dst.sampleImage.SetImageLayout(vk::ImageLayout::eGeneral);
        dst.weightImage.SetImageLayout(vk::ImageLayout::eGeneral);
    }

    void CopyFromResevImages(ResevImages& src)
    {
        src.sampleImage.SetImageLayout(vk::ImageLayout::eTransferSrcOptimal);
        src.weightImage.SetImageLayout(vk::ImageLayout::eTransferSrcOptimal);
        oldResevImages.sampleImage.SetImageLayout(vk::ImageLayout::eTransferDstOptimal);
        oldResevImages.weightImage.SetImageLayout(vk::ImageLayout::eTransferDstOptimal);

        src.sampleImage.CopyToImage(oldResevImages.sampleImage);
        src.weightImage.CopyToImage(oldResevImages.weightImage);

        src.sampleImage.SetImageLayout(vk::ImageLayout::eGeneral);
        src.weightImage.SetImageLayout(vk::ImageLayout::eGeneral);
        oldResevImages.sampleImage.SetImageLayout(vk::ImageLayout::eGeneral);
        oldResevImages.weightImage.SetImageLayout(vk::ImageLayout::eGeneral);
    }

private:
    ResevImages newResevImages;
    ResevImages oldResevImages;
};

class ShadingPipeline : public RayTracingPipeline
{
public:
    ShadingPipeline(const Scene& scene, const GBuffers& gbuffers, const ResevImages& resevImages, size_t pushSize = 0)
    {
        descSet = std::make_shared<DescriptorSet>();
        RayTracingPipeline::LoadShaders(SHADER_DIR + "restir/shading.rgen",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                        SHADER_DIR + "shadow_ray/shadow_ray.rchit");
        Register("topLevelAS", scene.GetAccel());
        Register("samplers", scene.GetTextures());
        Register("Addresses", scene.GetAddressBuffer());

        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);
        Register("resevSampleImage", resevImages.sampleImage);
        Register("resevWeightImage", resevImages.weightImage);

        outputImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm };
        Register("outputImage", outputImage);

        RayTracingPipeline::Setup(pushSize);
    }

    const Image& GetOutputImage() const { return outputImage; }

private:
    Image outputImage;
};
