#include "StarBuffer.hpp"
/*
* Initially based off Ive_buffer by Brendan Galea -
*https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
*/

namespace star {
vk::DeviceSize StarBuffer::GetAlignment(vk::DeviceSize instanceSize, vk::DeviceSize minOffsetAlignment) {
	if (minOffsetAlignment > 0) {
		return  (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	}
	return instanceSize;
}

StarBuffer::StarBuffer(VmaAllocator& allocator, vk::DeviceSize instanceSize, uint32_t instanceCount,
	const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
	const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode, const std::string& allocationName,
	vk::DeviceSize minOffsetAlignment) 
	: allocator(allocator),
	usageFlags{useFlags},
	instanceSize{ instanceSize },
	instanceCount{ instanceCount }
{
	this->alignmentSize = GetAlignment(this->instanceSize, minOffsetAlignment);
	this->bufferSize = this->alignmentSize * instanceCount;

	if (bufferSize == 0)
		throw std::runtime_error("Unable to create a buffer of size 0");

	VmaAllocationInfo allocationInfo{}; 
	CreateBuffer(allocator, this->bufferSize, useFlags, memoryUsageFlags, creationFlags, this->buffer, this->memory, allocationInfo, allocationName);

	this->allocationInfo = std::make_unique<VmaAllocationInfo>(allocationInfo);
}

StarBuffer::StarBuffer(VmaAllocator& allocator, const StarBuffer::BufferCreationArgs& creationArgs)
	: allocator(allocator), 
	usageFlags(creationArgs.creationFlags), 
	instanceSize(creationArgs.instanceSize),
	instanceCount(creationArgs.instanceCount)
{
	this->alignmentSize = GetAlignment(this->instanceSize, creationArgs.minOffsetAlignment); 
	this->bufferSize = this->alignmentSize * this->instanceCount; 

	if (bufferSize == 0)
		throw std::runtime_error("Unable to create a buffer of size 0");

	VmaAllocationInfo allocationInfo{}; 
	CreateBuffer(
		allocator, 
		this->bufferSize, 
		creationArgs.useFlags, 
		creationArgs.memoryUsageFlags, 
		creationArgs.creationFlags, 
		this->buffer, 
		this->memory, 
		allocationInfo, 
		creationArgs.allocationName);

	this->allocationInfo = std::make_unique<VmaAllocationInfo>(allocationInfo);
}

StarBuffer::~StarBuffer() {
	if (mapped)
		vmaUnmapMemory(allocator, this->memory);

	vmaDestroyBuffer(allocator, this->buffer, this->memory);
}

void StarBuffer::map(vk::DeviceSize size, vk::DeviceSize offset) {
	assert(this->buffer && this->memory && "Called map on buffer before creation");

	vmaMapMemory(this->allocator, this->memory, &this->mapped);
}

void StarBuffer::unmap() {
	if (mapped) {
		vmaUnmapMemory(this->allocator, this->memory);
		this->mapped = nullptr;
	}
}

void StarBuffer::writeToBuffer(void* data, vk::DeviceSize size, vk::DeviceSize offset) {
	assert(this->mapped && "Cannot copy to unmapped buffer");

	if (size == vk::WholeSize) {
		memcpy(this->mapped, data, this->bufferSize);
	}
	else {
		char* memOffset = (char*)mapped;
		memOffset += offset;
		memcpy(memOffset, data, size);
	}
}

vk::Result StarBuffer::flush(vk::DeviceSize size, vk::DeviceSize offset) {
	auto result = vmaFlushAllocation(this->allocator, this->memory, offset, size);
	return vk::Result(result);
}

vk::DescriptorBufferInfo StarBuffer::descriptorInfo(vk::DeviceSize size, vk::DeviceSize offset) {
	return vk::DescriptorBufferInfo{
		buffer,
		offset,
		size
	};
}

void StarBuffer::writeToIndex(void* data, int index) {
	writeToBuffer(data, this->instanceSize, index * this->alignmentSize);
}

vk::Result StarBuffer::flushIndex(int index) {
	return flush(this->alignmentSize, index * this->alignmentSize);
}

vk::DescriptorBufferInfo StarBuffer::descriptorInfoForIndex(int index) {
	return descriptorInfo(this->alignmentSize, index * alignmentSize);
}

StarBuffer::StarBuffer(VmaAllocator &allocator, const uint32_t &instanceCount, const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment, const VmaAllocationCreateInfo &allocCreateInfo, const vk::BufferCreateInfo &bufferCreateInfo, const std::string &allocName)
: allocator(allocator), instanceCount(instanceCount), instanceSize(instanceSize), alignmentSize(GetAlignment(instanceSize, minOffsetAlignment)), bufferSize(bufferCreateInfo.size)
{
	VmaAllocationInfo nAllocInfo{}; 
	CreateBuffer(allocator, allocCreateInfo, bufferCreateInfo, allocName, this->buffer, this->memory, nAllocInfo);
	
	this->allocationInfo = std::make_unique<VmaAllocationInfo>(nAllocInfo); 
}

void StarBuffer::CreateBuffer(VmaAllocator &allocator, const vk::DeviceSize &size, const vk::BufferUsageFlags &usage, const VmaMemoryUsage &memoryUsage, const VmaAllocationCreateFlags &flags, vk::Buffer &buffer, VmaAllocation &memory, VmaAllocationInfo &allocationInfo, const std::string &allocationName)
{
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;
	allocInfo.flags = flags;

	auto result = (vk::Result)vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bufferInfo, &allocInfo, (VkBuffer*)&buffer, &memory, &allocationInfo);

	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Failed to allocate buffer memory");

	std::string fullAllocationName = std::string(allocationName);
	fullAllocationName += "_BUFFER";
	vmaSetAllocationName(allocator, memory, fullAllocationName.c_str());
}

void StarBuffer::CreateBuffer(VmaAllocator &allocator, const VmaAllocationCreateInfo &allocCreateInfo, const vk::BufferCreateInfo &bufferInfo, const std::string &allocationName, vk::Buffer &buffer, VmaAllocation &memory, VmaAllocationInfo &allocationInfo)
{	
	auto result = (vk::Result)vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bufferInfo, &allocCreateInfo, (VkBuffer*)&buffer, &memory, &allocationInfo); 

	if (result != vk::Result::eSuccess){
		throw std::runtime_error("Failed to allocate buffer memory"); 
	}

	vmaSetAllocationName(allocator, memory, allocationName.c_str()); 
}

StarBuffer::Builder::Builder(VmaAllocator &allocator)
: allocator(allocator)
{
}

StarBuffer::Builder &StarBuffer::Builder::setAllocationCreateInfo(const VmaAllocationCreateInfo &nAllocInfo, const vk::BufferCreateInfo &nBufferInfo, const std::string &nAllocName)
{
	this->allocInfo = nAllocInfo;
	this->allocName = nAllocName; 
	this->buffInfo = nBufferInfo;

	return *this; 
}

StarBuffer::Builder &StarBuffer::Builder::setInstanceCount(const uint32_t &nInstanceCount)
{
	this->instanceCount = nInstanceCount;
	return *this;
}

StarBuffer::Builder &StarBuffer::Builder::setInstanceSize(const vk::DeviceSize &nInstanceSize)
{
	this->instanceSize = nInstanceSize; 
	return *this;
}

StarBuffer::Builder &StarBuffer::Builder::setMinOffsetAlignment(const vk::DeviceSize &nMinOffsetAlignment)
{
	this->minOffsetAlignment = nMinOffsetAlignment; 
	return *this;
}

std::unique_ptr<StarBuffer> StarBuffer::Builder::build()
{
    assert(this->instanceCount != 0 && this->instanceSize != 0 && "Instance info must be provided"); 
	
	return std::unique_ptr<StarBuffer>(new StarBuffer(
		this->allocator, 
		this->instanceCount,
		this->instanceSize,
		this->minOffsetAlignment,
		this->allocInfo, 
		this->buffInfo, 
		this->allocName
	));
}
}