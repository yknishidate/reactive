#include "Buffer.hpp"
#include "Image.hpp"

class DescriptorSet
{
public:
    void Allocate(const std::string& shaderPath);
    void Update();

    void Register(const std::string& name, vk::Buffer buffer, size_t size);
    void Register(const std::string& name, vk::ImageView view, vk::Sampler sampler);
    void Register(const std::string& name, const std::vector<Image>& images);
    void Register(const std::string& name, const Buffer& buffer);
    void Register(const std::string& name, const Image& image);
    void Register(const std::string& name, const vk::AccelerationStructureKHR& accel);

private:
    vk::UniqueDescriptorSetLayout descSetLayout;
    vk::UniqueDescriptorSet descSet;
    std::unordered_map<std::string, vk::DescriptorSetLayoutBinding> bindingMap;

    std::vector<std::vector<vk::DescriptorImageInfo>> imageInfos;
    std::vector<std::vector<vk::DescriptorBufferInfo>> bufferInfos;
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelInfos;
    std::vector<vk::WriteDescriptorSet> writes;
};
