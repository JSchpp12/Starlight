#pragma once

#include "Enums.hpp"
#include "StarBuffer.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>
#include <array>
#include <queue>

namespace star {
	class BufferManager {
	public:
		static void registerDataToBuffer(star::Buffer_Type type, int imageInFlightIndex,
			unsigned int dataSize, void* data);

		void store(StarDevice& device, star::Buffer_Type type); 

		BufferManager(int imagesInFlight) : imagesInFlight(imagesInFlight){};

	protected:
		struct RecordData {
			int imageInFlightIndex; 
			unsigned int size; 
			void* data; 

			RecordData(unsigned int size, void* data) : size(size), data(data) {};
		};

		static std::queue<std::pair<star::Buffer_Type, RecordData>> newBufferRegistrations; 
		std::unique_ptr<star::StarBuffer> uniformUpdatePerFrame; 
		int imagesInFlight; 

			
	};
}