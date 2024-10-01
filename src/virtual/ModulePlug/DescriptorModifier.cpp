#include "DescriptorModifier.hpp"

void star::DescriptorModifier::submitToManager()
{
	auto requestCallback = std::function<std::vector<std::pair<vk::DescriptorType, const int>>(const int&)>(std::bind(&DescriptorModifier::getDescriptorRequests, this, std::placeholders::_1));
	
	ManagerDescriptorPool::request(requestCallback);
}
