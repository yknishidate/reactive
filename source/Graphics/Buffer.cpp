#include "Buffer.hpp"

Buffer::Buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size)
	: size(size)
{
	buffer = Graphics::GetDevice().createBufferUnique({ {}, size, usage });

	vk::MemoryRequirements requirements = Graphics::GetDevice().getBufferMemoryRequirements(*buffer);
	uint32_t memoryTypeIndex = Graphics::FindMemoryTypeIndex(requirements, memoryProp);
	vk::MemoryAllocateFlagsInfo flagsInfo{ vk::MemoryAllocateFlagBits::eDeviceAddress };
	memory = Graphics::GetDevice().allocateMemoryUnique(
		vk::MemoryAllocateInfo()
		.setAllocationSize(requirements.size)
		.setMemoryTypeIndex(memoryTypeIndex)
		.setPNext(&flagsInfo));

	Graphics::GetDevice().bindBufferMemory(*buffer, *memory, 0);

	if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
		vk::BufferDeviceAddressInfoKHR bufferDeviceAI{ *buffer };
		deviceAddress = Graphics::GetDevice().getBufferAddressKHR(&bufferDeviceAI);
	}
}

HostBuffer::HostBuffer(vk::BufferUsageFlags usage, size_t size)
	: Buffer(usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, size)
{
}

void HostBuffer::Copy(const void* data)
{
	Map();
	std::memcpy(mapped, data, size);
}

void* HostBuffer::Map()
{
	if (!mapped) {
		mapped = Graphics::GetDevice().mapMemory(*memory, 0, VK_WHOLE_SIZE);
	}
	return mapped;
}

DeviceBuffer::DeviceBuffer(vk::BufferUsageFlags usage, size_t size)
	: Buffer(usage, vk::MemoryPropertyFlagBits::eDeviceLocal, size)
{
}

void DeviceBuffer::Copy(const void* data)
{
	StagingBuffer stagingBuffer{ size, data };
	Graphics::OneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
		vk::BufferCopy region{ 0, 0, size };
	commandBuffer.copyBuffer(stagingBuffer.GetBuffer(), *buffer, region);
		});
}

StagingBuffer::StagingBuffer(size_t size, const void* data)
	: HostBuffer{ Usage::eTransferSrc, size }
{
	Copy(data);
}
