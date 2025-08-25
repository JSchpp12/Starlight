#include "ManagerDescriptorPool.hpp"

bool star::core::devices::managers::ManagerDescriptorPool::ready = false; 
std::stack<std::function<std::vector<std::pair<vk::DescriptorType, const int>>(const int&)>> star::core::devices::managers::ManagerDescriptorPool::requestCallbacks = std::stack < std::function<std::vector<std::pair<vk::DescriptorType, const int>>(const int&)>>();
std::stack<std::function<void(star::core::devices::DeviceContext&, const int&)>> star::core::devices::managers::ManagerDescriptorPool::creationCallbacks = std::stack<std::function<void(star::core::devices::DeviceContext&, const int&)>>(); 
std::unordered_map<vk::DescriptorType, int> star::core::devices::managers::ManagerDescriptorPool::actives = std::unordered_map<vk::DescriptorType, int>(); 
star::StarDescriptorPool* star::core::devices::managers::ManagerDescriptorPool::pool = nullptr; 

void star::core::devices::managers::ManagerDescriptorPool::request(std::function<std::vector<std::pair<vk::DescriptorType, const int>>(const int&)> newRequest, 
	std::function<void(star::core::devices::DeviceContext&, const int&)> createCall)
{
	requestCallbacks.push(newRequest);
	creationCallbacks.push(createCall);
}

star::StarDescriptorPool& star::core::devices::managers::ManagerDescriptorPool::getPool()
{
	assert(pool != nullptr && "The manager must BUILD the pool before it can be used"); 

	return *pool; 
}

star::core::devices::managers::ManagerDescriptorPool::ManagerDescriptorPool(core::devices::DeviceContext& device, const int& numFramesInFligth)
: device(device) 
{
	init(numFramesInFligth);
}

star::core::devices::managers::ManagerDescriptorPool::~ManagerDescriptorPool()
{
	currentPool.reset();
}

void star::core::devices::managers::ManagerDescriptorPool::init(const int& numFramesInFlight)
{
	while (!requestCallbacks.empty()) {
		std::function<std::vector<std::pair<vk::DescriptorType, const int>>(const int&)> callback = requestCallbacks.top();
		
		auto requests = callback(numFramesInFlight);

		for (auto& request : requests) {
			//check if type is already in active pool
			if (actives.count(request.first) != 0) {
				auto num = this->actives.at(request.first);
				num += request.second;

				this->actives.at(request.first) = num;
			}
			else {
				this->actives.insert(std::pair<vk::DescriptorType, int>(request));
			}
		}
		
		requestCallbacks.pop();
	}

	//build from actives 
	auto builder = StarDescriptorPool::Builder(device.getDevice()); 
	int total = 0;
	for (auto& active : this->actives) {
		int numToAdd = active.second * 3;
		total += numToAdd; 
		builder.addPoolSize(active.first, numToAdd); 
	}
	builder.setMaxSets(total);

	ready = true; 

	this->currentPool = builder.build();
	pool = this->currentPool.get(); 
}

void star::core::devices::managers::ManagerDescriptorPool::update(const int& numFramesInFlight)
{
	while (!creationCallbacks.empty()) {
		std::function<void(star::core::devices::DeviceContext&, const int&)> callback = creationCallbacks.top();
		callback(this->device, numFramesInFlight);
		creationCallbacks.pop();
	}
}
