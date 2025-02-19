// #pragma once

// #include "ManagerBuffer.hpp"
// #include "StarBuffer.hpp"
// #include "StarDevice.hpp"
// #include "Handle.hpp"
// #include "ManagerPlug.hpp"

// #include <vulkan/vulkan.hpp>

// #include <memory>

// namespace star {
// 	class BufferModifier : public ManagerPlug{
// 	public:
// 		BufferModifier(const VmaAllocationCreateFlags& createFlags, const VmaMemoryUsage& memoryUsage,
// 			const vk::BufferUsageFlagBits& bufferUsageFlags, const vk::DeviceSize& bufferSize,
// 			const int& instanceCount, const vk::SharingMode& bufferSharingMode,
// 			const int& minOffsetAlignment, const uint16_t& frameInFlightIndexToUpdateOn) 
// 		{
// 			init(createFlags, memoryUsage, bufferUsageFlags, bufferSize, instanceCount, bufferSharingMode, minOffsetAlignment, frameInFlightIndexToUpdateOn);
// 		}

// 		BufferModifier(const VmaAllocationCreateFlags& createFlags, const VmaMemoryUsage& memoryUsage,
// 			const vk::BufferUsageFlagBits& bufferUsageFlags, const vk::DeviceSize& bufferSize,
// 			const int& instanceCount, const vk::SharingMode& bufferSharingMode,
// 			const int& minOffsetAlignment) {
// 			init(createFlags, memoryUsage, bufferUsageFlags, bufferSize, instanceCount, bufferSharingMode, minOffsetAlignment);
// 		}

// 		bool getBufferHasChanged() const {
// 			return this->bufferHasChanged;
// 		}

// 		~BufferModifier(); 
// 	protected:
// 		void init(const VmaAllocationCreateFlags& createFlags, const VmaMemoryUsage& memoryUsage,
// 			const vk::BufferUsageFlagBits& bufferUsageFlags, const vk::DeviceSize& bufferSize,
// 			const int& instanceCount, const vk::SharingMode& bufferSharingMode,
// 			const int& minOffsetAlignment, const uint16_t& frameInFlightIndexToUpdateOn);

// 		void init(const VmaAllocationCreateFlags& createFlags, const VmaMemoryUsage& memoryUsage,
// 			const vk::BufferUsageFlagBits& bufferUsageFlags, const vk::DeviceSize& bufferSize,
// 			const int& instanceCount, const vk::SharingMode& bufferSharingMode,
// 			const int& minOffsetAlignment); 

// 		virtual void writeBufferData(StarBuffer& buffer) = 0; 

// 		virtual bool checkIfShouldUpdateThisFrame() {
// 			return true; 
// 		}

// 	private:
// 		bool bufferHasChanged = false; 

// 		void setBufferHasChanged() {
// 			this->bufferHasChanged = true;
// 		}


// 	};
// }