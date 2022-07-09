#pragma once
#include "Vulkan/Pipeline.hpp"

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
    GBufferPipeline(const Scene& scene, size_t pushSize = 0);

    const GBuffers& GetGBuffers() const { return gbuffers; }

private:
    GBuffers gbuffers;
};

class UniformLightPipeline : public RayTracingPipeline
{
public:
    UniformLightPipeline(const Scene& scene, const GBuffers& gbuffers, size_t pushSize = 0);

    const Image& GetOutputImage() const { return outputImage; }

private:
    Image outputImage;
};

class WRSPipeline : public RayTracingPipeline
{
public:
    WRSPipeline(const Scene& scene, const GBuffers& gbuffers, size_t pushSize = 0);

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
    InitResevPipeline(const Scene& scene, const GBuffers& gbuffers, size_t pushSize = 0);

    ResevImages& GetResevImages() { return resevImages; }

private:
    ResevImages resevImages;
};

class SpatialReusePipeline : public RayTracingPipeline
{
public:
    SpatialReusePipeline(const Scene& scene, const GBuffers& gbuffers, const ResevImages& resevImages, size_t pushSize = 0);

    ResevImages& GetNewResevImages() { return newResevImages; }

    void CopyToResevImages(ResevImages& dst);

private:
    ResevImages newResevImages;
};

class TemporalReusePipeline : public RayTracingPipeline
{
public:
    TemporalReusePipeline(const Scene& scene, const GBuffers& gbuffers, const ResevImages& resevImages, size_t pushSize = 0);

    ResevImages& GetNewResevImages() { return newResevImages; }

    void CopyToResevImages(ResevImages& dst);
    void CopyFromResevImages(ResevImages& src);

private:
    ResevImages newResevImages;
    ResevImages oldResevImages;
};

class ShadingPipeline : public RayTracingPipeline
{
public:
    ShadingPipeline(const Scene& scene, const GBuffers& gbuffers, const ResevImages& resevImages, size_t pushSize = 0);

    const Image& GetOutputImage() const { return outputImage; }

private:
    Image outputImage;
};
