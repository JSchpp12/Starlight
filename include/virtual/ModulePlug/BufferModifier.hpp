#pragma once

#include "ManagerBuffer.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "Handle.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star {
	class BufferModifier {
	public:
		BufferModifier(const VmaAllocationCreateFlags& createFlags, const VmaMemoryUsage& memoryUsage,
			const vk::BufferUsageFlagBits& bufferUsageFlags, const vk::DeviceSize& bufferSize,
			const int& instanceCount, const vk::SharingMode& bufferSharingMode, const uint16_t& frameInFlightIndexToUpdateOn,
			const bool& willBeWrittenToOnce, const int& minOffsetAlignment); 

		Handle& getHandle() const {
			return *this->bufferHandle;
		};

		~BufferModifier(); 
	protected:
		virtual void writeBufferData(StarBuffer& buffer) = 0; 

	private:
		std::unique_ptr<Handle> bufferHandle = nullptr; 
	};
}