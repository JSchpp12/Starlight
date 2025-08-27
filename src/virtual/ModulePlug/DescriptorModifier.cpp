#include "DescriptorModifier.hpp"

#include "ManagerDescriptorPool.hpp"

void star::DescriptorModifier::submitToManager()
{
	auto requestCallback = std::function<std::vector<std::pair<vk::DescriptorType, const int>>(const int&)>(std::bind(&DescriptorModifier::getDescriptorRequests, this, std::placeholders::_1));
	auto createCallback = std::function<void(core::device::DeviceContext&, const int&)>(std::bind(&DescriptorModifier::createDescriptors, this, std::placeholders::_1, std::placeholders::_2));

	core::device::managers::ManagerDescriptorPool::request(requestCallback, createCallback);
}
