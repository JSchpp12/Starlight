#pragma once

#include "StarDescriptors.hpp"

#include <vulkan/vulkan.hpp>

#include <stack>
#include <unordered_map>
#include <memory>

namespace star {
	class ManagerDescriptorPool {
	public:
		static void request(const vk::DescriptorType& type, const int& numDescriptors);

		static StarDescriptorPool& getPool();

		~ManagerDescriptorPool();

		void build(StarDevice& device);

	private:
		//are the pools ready for use?
		static bool ready; 
		static std::stack<std::pair<vk::DescriptorType, int>> requests;

		static StarDescriptorPool* pool; 

		std::unique_ptr<StarDescriptorPool> currentPool; 

		static std::unordered_map<vk::DescriptorType, int> actives; 

	};
}