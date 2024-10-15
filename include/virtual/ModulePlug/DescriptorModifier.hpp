#pragma once

#include "ManagerDescriptorPool.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <functional>

namespace star {
	class DescriptorModifier {
	public:
		DescriptorModifier() {
			this->submitToManager(); 
		}

	protected:
		virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) = 0; 

		virtual void createDescriptors(star::StarDevice& device, const int& numFramesInFlight) = 0; 

	private:
		void submitToManager(); 

	};
}