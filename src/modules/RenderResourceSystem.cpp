#include "RenderResourceSystem.hpp"

std::stack<std::function<void(star::StarDevice&, const int)>> star::RenderResourceSystem::initCallbacks = std::stack<std::function<void(StarDevice&, int)>>();
std::stack<std::function<void(star::StarDevice&)>> star::RenderResourceSystem::destroyCallbacks = std::stack<std::function<void(star::StarDevice&)>>();
std::stack<std::function<std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>(star::StarDevice&, star::BufferHandle, star::BufferHandle)>> star::RenderResourceSystem::loadGeometryCallbacks = std::stack<std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, BufferHandle, BufferHandle)>>();
std::vector<std::unique_ptr<star::StarBuffer>> star::RenderResourceSystem::buffers = std::vector<std::unique_ptr<star::StarBuffer>>(); 

void star::RenderResourceSystem::registerCallbacks(std::function<void(star::StarDevice&, const int)> initCallback, std::function<void(star::StarDevice&)> destroyCallback)
{
	initCallbacks.push(initCallback); 
	destroyCallbacks.push(destroyCallback);
}

void star::RenderResourceSystem::registerLoadGeomDataCallback(std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, BufferHandle, BufferHandle)> loadGeometryCallback)
{
	loadGeometryCallbacks.push(loadGeometryCallback); 
}

void star::RenderResourceSystem::bind(const Handle& resource, vk::CommandBuffer& commandBuffer)
{
	switch (resource.type) {
	case(Handle_Type::buffer):
		bindBuffer(resource.id, commandBuffer);
		break;
	default:
		throw std::runtime_error("Unsupported resource type requested for bind operation " + resource.type);
	}
}

void star::RenderResourceSystem::bind(const BufferHandle& buffer, vk::CommandBuffer& commandBuffer)
{
	bindBuffer(buffer.id, commandBuffer, buffer.targetBufferOffset); 
}

void star::RenderResourceSystem::cleanup(StarDevice& device)
{
	runDestroys(device);

	for (auto& buffer : buffers) {
		buffer.reset(); 
	}
}

void star::RenderResourceSystem::preparePrimaryGeometry(StarDevice& device)
{
	vk::DeviceSize totalVertSize = 0, totalVertInstanceCount = 0, totalIndSize = 0, totalIndInstanceCount = 0;
	std::vector<std::unique_ptr<StarBuffer>> stagingBuffersVert, stagingBuffersIndex; 

	while (!loadGeometryCallbacks.empty()) {
		BufferHandle vertBufferHandle = BufferHandle{ 0, totalVertSize };
		BufferHandle indBufferHandle = BufferHandle{ 1 };

		std::function<std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>(star::StarDevice&, star::BufferHandle, star::BufferHandle)>& function = loadGeometryCallbacks.top();
		std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>> result = function(device, vertBufferHandle, indBufferHandle);

		//might want to throw a check in here to verify buffer type
		totalVertSize += result.first->getBufferSize();
		totalVertInstanceCount += result.first->getInstanceCount();
		totalIndSize += result.second->getInstanceSize();
		totalIndInstanceCount += result.second->getInstanceCount();
		stagingBuffersVert.push_back(std::move(result.first));
		stagingBuffersIndex.push_back(std::move(result.second));

		loadGeometryCallbacks.pop();
	}

	vk::DeviceSize vertexSize = sizeof(star::Vertex);
	std::unique_ptr<StarBuffer> vertBuffer = std::make_unique<StarBuffer>(
		device, 
		vertexSize, 
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

	vk::DeviceSize indSize = sizeof(uint32_t);
	std::unique_ptr<StarBuffer> indBuffer = std::make_unique<StarBuffer>(
		device,
		indSize,
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

void star::RenderResourceSystem::bindBuffer(const uint32_t& bufferId, vk::CommandBuffer& commandBuffer, const size_t& offset)
{
	assert(bufferId < buffers.size() && "Handle provided has an id which does not map to any resources");
	
	StarBuffer& buffer = *buffers.at(bufferId);
	vk::DeviceSize vOffset{ offset };

	if (buffer.getUsageFlags() & vk::BufferUsageFlagBits::eVertexBuffer) {
		commandBuffer.bindVertexBuffers(0, buffer.getBuffer(), vOffset);
	}
	else if (buffer.getUsageFlags() & vk::BufferUsageFlagBits::eIndexBuffer)
		commandBuffer.bindIndexBuffer(buffer.getBuffer(), vOffset, vk::IndexType::eUint32);
	else
		throw std::runtime_error("Unsupported buffer type requested for bind operation from Resource System");
}
