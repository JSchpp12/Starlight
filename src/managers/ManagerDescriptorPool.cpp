#include "ManagerDescriptorPool.hpp"

bool star::ManagerDescriptorPool::ready = false; 
std::stack<std::pair<vk::DescriptorType, int>> star::ManagerDescriptorPool::requests = std::stack<std::pair<vk::DescriptorType, int>>(); 
std::unordered_map<vk::DescriptorType, int> star::ManagerDescriptorPool::actives = std::unordered_map<vk::DescriptorType, int>(); 
star::StarDescriptorPool* star::ManagerDescriptorPool::pool = nullptr; 

void star::ManagerDescriptorPool::request(const vk::DescriptorType& type, const int& numDescriptors) {
	requests.push(std::pair<vk::DescriptorType, int>(type, numDescriptors));
}

star::StarDescriptorPool& star::ManagerDescriptorPool::getPool()
{
	assert(pool != nullptr && "The manager must BUILD the pool before it can be used"); 

	return *pool; 
}

star::ManagerDescriptorPool::~ManagerDescriptorPool()
{
	currentPool.reset();
}

void star::ManagerDescriptorPool::build(star::StarDevice& device)
{
	while (!requests.empty()) {
		std::pair<vk::DescriptorType, int>& request = requests.top();
		
		//check if type is already in active pool
		if (actives.count(request.first) != 0) {
			auto num = this->actives.at(request.first);
			num += request.second;

			this->actives.at(request.first) = num;
		}
		else {
			this->actives.insert(std::pair<vk::DescriptorType, int>(request));
		}

		requests.pop();
	}

	//build from actives 
	auto builder = StarDescriptorPool::Builder(device); 
	for (auto& active : this->actives) {
		builder.addPoolSize(active.first, active.second); 
	}

	ready = true; 

	this->currentPool = builder.build();
	pool = this->currentPool.get(); 
}
