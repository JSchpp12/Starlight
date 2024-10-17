#include "BufferModifier.hpp"

star::BufferModifier::~BufferModifier()
{
	ManagerBuffer::destroy(*this->bufferHandle);
}

star::BufferModifier::BufferModifier(const VmaAllocationCreateFlags& createFlags, const VmaMemoryUsage& memoryUsage,
	const vk::BufferUsageFlagBits& bufferUsageFlags, const vk::DeviceSize& bufferSize,
	const int& instanceCount, const vk::SharingMode& bufferSharingMode, const uint16_t& frameInFlightIndexToUpdateOn,
	const bool& willBeWrittenToOnce, const int& minOffsetAlignment)
{

	this->bufferHandle = std::make_unique<Handle>(
		ManagerBuffer::addRequest(ManagerBuffer::Request(
			std::function<void(StarBuffer&)>(std::bind(&BufferModifier::writeBufferData, this, std::placeholders::_1)),
			willBeWrittenToOnce,
			bufferSize,
			instanceCount,
			createFlags,
			memoryUsage,
			bufferUsageFlags,
			bufferSharingMode,
			minOffsetAlignment,
			frameInFlightIndexToUpdateOn
		)));
}