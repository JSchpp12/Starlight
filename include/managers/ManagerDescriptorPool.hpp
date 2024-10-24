#pragma once

#include "StarDescriptorBuilders.hpp"

#include <vulkan/vulkan.hpp>

#include <stack>
#include <unordered_map>
#include <memory>

namespace star {
	class ManagerDescriptorPool {
	public:
		static void request(std::function<std::vector<std::pair<vk::DescriptorType, const int>>(const int&)> newRequest, 
			std::function<void(StarDevice&, const int&)> createCall);

		static StarDescriptorPool& getPool();

		ManagerDescriptorPool(StarDevice& device, const int& numFramesInFligth); 

		~ManagerDescriptorPool();

		void init(const int& numFramesInFlight);

		void update(const int& numFramesInFlight); 

	private:
		//are the pools ready for use?
		static bool ready; 
		//static std::stack<std::pair<vk::DescriptorType, int>> requests;
		static std::stack<std::function<std::vector<std::pair<vk::DescriptorType, const int>>(const int&)>> requestCallbacks;
		static std::stack<std::function<void(StarDevice&, const int&)>> creationCallbacks; 

		static StarDescriptorPool* pool; 

		StarDevice& device; 
		std::unique_ptr<StarDescriptorPool> currentPool; 

		static std::unordered_map<vk::DescriptorType, int> actives; 

	};
}