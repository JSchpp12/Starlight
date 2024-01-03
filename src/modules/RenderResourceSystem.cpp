#include "RenderResourceSystem.hpp"

std::stack<std::function<void(star::StarDevice&, const int)>> star::RenderResourceSystem::initCallbacks = std::stack<std::function<void(StarDevice&, int)>>();
std::stack<std::function<void(star::StarDevice&)>> star::RenderResourceSystem::destroyCallbacks = std::stack<std::function<void(star::StarDevice&)>>();
std::stack<std::function<std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>(star::StarDevice&, star::Handle&, star::Handle&)>> star::RenderResourceSystem::loadGeometryCallbacks = std::stack<std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, Handle&, Handle&)>>();
std::vector<std::unique_ptr<star::StarBuffer>> star::RenderResourceSystem::buffers = std::vector<std::unique_ptr<star::StarBuffer>>(); 

void star::RenderResourceSystem::registerCallbacks(std::function<void(star::StarDevice&, const int)> initCallback, std::function<void(star::StarDevice&)> destroyCallback)
{
	initCallbacks.push(initCallback); 
	destroyCallbacks.push(destroyCallback);
}

void star::RenderResourceSystem::registerLoadGeomDataCallback(std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, Handle&, Handle&)> loadGeometryCallback)
{
	loadGeometryCallbacks.push(loadGeometryCallback); 
}

void star::RenderResourceSystem::bind(const Handle& resource, vk::CommandBuffer& commandBuffer)
{
	switch (resource.type) {
	case(Handle_Type::buffer):
		assert(resource.id < buffers.size() && "Handle provided has an id which does not map to any resources");
		{
			StarBuffer& buffer = *buffers.at(resource.id);
			if (buffer.getUsageFlags() & vk::BufferUsageFlagBits::eVertexBuffer) {
				vk::DeviceSize offset{};
				commandBuffer.bindVertexBuffers(0, buffer.getBuffer(), offset);
			}
			else if (buffer.getUsageFlags() & vk::BufferUsageFlagBits::eIndexBuffer)
				commandBuffer.bindIndexBuffer(buffer.getBuffer(), {}, vk::IndexType::eUint32);
			else
				throw std::runtime_error("Unsupported buffer type requested for bind operation from Resource System");
		}

		break;
	default:
		throw std::runtime_error("Unsupported resource type requested for bind operation " + resource.type);
	}
}

void star::RenderResourceSystem::preparyPrimaryGeometry(StarDevice& device)
{
	vk::DeviceSize totalVertSize = 0, totalVertInstanceCount = 0, totalIndSize = 0, totalIndInstanceCount = 0;
	std::vector<std::unique_ptr<StarBuffer>> stagingBuffersVert, stagingBuffersIndex; 
	star::Handle primaryVertBuffer{ 0, Handle_Type::buffer }, primaryIndBuffer{1, Handle_Type::buffer};

	while (!loadGeometryCallbacks.empty()) {
		std::function<std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>(star::StarDevice&, star::Handle&, star::Handle&)>& function = loadGeometryCallbacks.top();
		std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>> result = function(device, primaryVertBuffer, primaryIndBuffer);

		if (result.first->getUsageFlags() & vk::BufferUsageFlagBits::eVertexBuffer && result.second->getUsageFlags() & vk::BufferUsageFlagBits::eIndexBuffer) {
			totalVertSize += result.first->getBufferSize();
			totalVertInstanceCount += result.first->getInstanceCount();
			totalIndSize += result.second->getInstanceSize(); 
			totalIndInstanceCount += result.second->getInstanceCount();
			stagingBuffersVert.push_back(std::move(result.first)); 
			stagingBuffersIndex.push_back(std::move(result.second)); 
		}
		else {
			totalVertSize += result.second->getBufferSize(); 
			totalVertInstanceCount += result.second->getInstanceCount(); 
			totalIndSize += result.first->getBufferSize(); 
			totalIndInstanceCount += result.first->getInstanceCount();
			stagingBuffersVert.push_back(std::move(result.second)); 
			stagingBuffersIndex.push_back(std::move(result.first));
		}

		loadGeometryCallbacks.pop();
	}

	vk::DeviceSize vBuffSize = totalVertInstanceCount * sizeof(star::Vertex);
	std::unique_ptr<StarBuffer> vertBuffer = std::make_unique<StarBuffer>(
		device, 
		vBuffSize, 
		totalVertInstanceCount,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, 
		vk::MemoryPropertyFlagBits::eDeviceLocal
	); 
	{
		vk::DeviceSize currentOffset = 0;
		for (auto& vertStage : stagingBuffersVert) {
			device.copyBuffer(vertStage->getBuffer(), vertBuffer->getBuffer(), vertStage->getBufferSize(), currentOffset);
			currentOffset += vertStage->getBufferSize();
		}
	}

	vk::DeviceSize iBuffSize = totalIndInstanceCount * sizeof(uint32_t);
	std::unique_ptr<StarBuffer> indBuffer = std::make_unique<StarBuffer>(
		device,
		iBuffSize,
		totalIndInstanceCount,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);
	{
		vk::DeviceSize currentOffset = 0; 
		for (auto& indStage : stagingBuffersIndex) {
			device.copyBuffer(indStage->getBuffer(), indBuffer->getBuffer(), indStage->getBufferSize(), currentOffset);

			currentOffset += indStage->getBufferSize(); 
		}
	}

	buffers.push_back(std::move(vertBuffer));
	buffers.push_back(std::move(indBuffer));
}

void star::RenderResourceSystem::runInits(StarDevice& device, const int numFramesInFlight)
{
	while (!initCallbacks.empty()) {
		std::function<void(StarDevice&, const int)>& function = initCallbacks.top();
		function(device, numFramesInFlight); 
		initCallbacks.pop(); 
	}
}

void star::RenderResourceSystem::runDestroys(StarDevice& device)
{
	while (!destroyCallbacks.empty()) {
		std::function<void(StarDevice&)>& function = destroyCallbacks.top();
		function(device);
		destroyCallbacks.pop();
	}
}
