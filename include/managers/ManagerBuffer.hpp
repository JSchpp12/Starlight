//#pragma once
//
//#include "Enums.hpp"
//#include "StarBuffer.hpp"
//#include "StarDevice.hpp"
//
//#include <vulkan/vulkan.hpp>
//
//#include <memory>
//#include <vector>
//#include <array>
//#include <queue>
//#include <functional>
//
//namespace star {
//	class BufferManager {
//	public:
//		struct CompleteRequest {
//			std::unique_ptr<StarBuffer> buffer;
//			std::function<void()> updateBufferData = std::function<void(void)>(); 
//			bool hasBeenUpdatedThisFrame = false; 
//
//			CompleteRequest(std::unique_ptr<StarBuffer> buffer, std::function<void(int)> updateBufferData)
//				: buffer(std::move(buffer)), updateBufferData(updateBufferData) {};
//		};
//
//		BufferManager(StarDevice& device, const int& numFramesInFlight)
//			: device(device), numFramesInFlight(numFramesInFlight) {};
//
//		void addRequest(const vk::DeviceSize& bufferSize, );
//
//	protected:
//		StarDevice& device;
//		int numFramesInFlight; 
//
//		std::vector<std::unique_ptr<StarBuffer>> allBuffers = std::vector<std::unique_ptr<StarBuffer>>();
//
//	};
//}