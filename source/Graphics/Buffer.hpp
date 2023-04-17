#pragma once
#include "App.hpp"

enum class BufferUsage {
    Vertex,
    Index,
    AccelInput,
    AccelStorage,
    Scratch,
    ShaderBindingTable,
    Staging,
    Storage,
    Uniform,
};

class Buffer {
public:
    Buffer() = default;

    vk::Buffer getBuffer() const { return *buffer; }
    vk::DeviceSize getSize() const { return size; }
    uint64_t getAddress() const {
        vk::BufferDeviceAddressInfoKHR bufferDeviceAI{*buffer};
        return m_app->getDevice().getBufferAddressKHR(&bufferDeviceAI);
    }
    vk::DescriptorBufferInfo getInfo() const { return {*buffer, 0, size}; }

protected:
    Buffer(const App* app, BufferUsage usage, vk::MemoryPropertyFlags memoryProp, size_t size);

    const App* m_app;
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size = 0u;
};

class HostBuffer : public Buffer {
public:
    HostBuffer() = default;
    HostBuffer(const App* app, BufferUsage usage, size_t size);

    void copy(const void* data);
    void* map();

private:
    void* mapped = nullptr;
};

class DeviceBuffer : public Buffer {
public:
    DeviceBuffer() = default;
    DeviceBuffer(const App* app, BufferUsage usage, size_t size);

    template <typename T>
    DeviceBuffer(const App* app, BufferUsage usage, const std::vector<T>& data)
        : DeviceBuffer(app, usage, sizeof(T) * data.size()) {
        copy(data.data());
    }

    void copy(const void* data);
};
