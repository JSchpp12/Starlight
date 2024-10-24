#include "BufferModifier.hpp"

star::BufferModifier::~BufferModifier()
{
	ManagerBuffer::destroy(*this->bufferHandle);
}

void star::BufferModifier::init(const VmaAllocationCreateFlags& createFlags, const VmaMemoryUsage& memoryUsage,
	const vk::BufferUsageFlagBits& bufferUsageFlags, const vk::DeviceSize& bufferSize,
	const int& instanceCount, const vk::SharingMode& bufferSharingMode,
	const int& minOffsetAlignment, const uint16_t& frameInFlightIndexToUpdateOn) {

	if (this->bufferHandle)
		ManagerBuffer::destroy(*this->bufferHandle);

	this->bufferHandle = std::make_unique<Handle>(
		ManagerBuffer::addRequest(ManagerBuffer::Request(
			std::function<void(StarBuffer&)>(std::bind(&BufferModifier::writeBufferData, this, std::placeholders::_1)),
			std::function<void(void)>(std::bind(&BufferModifier::setBufferHasChanged, this)),
			ManagerBuffer::BufferCreationArgs{
				bufferSize,
				static_cast<uint32_t>(instanceCount),
				createFlags,
				memoryUsage,
				bufferUsageFlags,
				bufferSharingMode,
				vk::DeviceSize(minOffsetAlignment)
			},
			frameInFlightIndexToUpdateOn, 
			std::function<bool(void)>((std::bind(&BufferModifier::checkIfShouldUpdateThisFrame, this)))
		)));
}

void star::BufferModifier::init(const VmaAllocationCreateFlags& createFlags, const VmaMemoryUsage& memoryUsage,
	const vk::BufferUsageFlagBits& bufferUsageFlags, const vk::DeviceSize& bufferSize,
	const int& instanceCount, const vk::SharingMode& bufferSharingMode,
	const int& minOffsetAlignment)
{
	if (this->bufferHandle)
		ManagerBuffer::destroy(*this->bufferHandle); 

	this->bufferHandle = std::make_unique<Handle>(
		ManagerBuffer::addRequest(ManagerBuffer::Request(
			std::function<void(StarBuffer&)>(std::bind(&BufferModifier::writeBufferData, this, std::placeholders::_1)),
			std::function<void(void)>(std::bind(&BufferModifier::setBufferHasChanged, this)),
			ManagerBuffer::BufferCreationArgs{
				bufferSize,
				static_cast<uint32_t>(instanceCount),
				createFlags,
				memoryUsage,
				bufferUsageFlags,
				bufferSharingMode,
				vk::DeviceSize(minOffsetAlignment)
			}
		)));
}