#include "RenderResourceSystem.hpp"

std::stack<std::function<void(star::StarDevice&, const int&, const vk::Extent2D&)>> star::RenderResourceSystem::initCallbacks = std::stack<std::function<void(StarDevice&, const int&, const vk::Extent2D&)>>();
std::stack<std::function<void(star::StarDevice&)>> star::RenderResourceSystem::destroyCallbacks = std::stack<std::function<void(star::StarDevice&)>>();
std::stack<std::function<std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>(star::StarDevice&, star::BufferHandle, star::BufferHandle)>> star::RenderResourceSystem::loadGeometryCallbacks = std::stack<std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, BufferHandle, BufferHandle)>>();
std::stack<std::function<void(const uint32_t&, const uint32_t&, const uint32_t&)>> star::RenderResourceSystem::geometryDataOffsetCallbacks = std::stack<std::function<void(const uint32_t&, const uint32_t&, const uint32_t&)>>();
std::vector<std::unique_ptr<star::StarBuffer>> star::RenderResourceSystem::buffers = std::vector<std::unique_ptr<star::StarBuffer>>(); 

void star::RenderResourceSystem::registerCallbacks(std::function<void(star::StarDevice&, const int&, const vk::Extent2D&)> initCallback, std::function<void(star::StarDevice&)> destroyCallback)
{
	initCallbacks.push(initCallback); 
	destroyCallbacks.push(destroyCallback);
}

void star::RenderResourceSystem::registerLoadGeomDataCallback(std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, BufferHandle, BufferHandle)> loadGeometryCallback)
{
	loadGeometryCallbacks.push(loadGeometryCallback); 
}

void star::RenderResourceSystem::registerSetDrawInfoCallback(std::function<void(const uint32_t&, const uint32_t&, const uint32_t&)> setGeometryDataOffsetCallback)
{
	geometryDataOffsetCallbacks.push(setGeometryDataOffsetCallback);
}

void star::RenderResourceSystem::bind(const Handle& resource, vk::CommandBuffer& commandBuffer)
{
	switch (resource.getType()) {
	case(Handle_Type::buffer):
		bindBuffer(resource.getID(), commandBuffer);
		break;
	default:
		throw std::runtime_error("Unsupported resource type requested for bind operation " + resource.getType());
	}
}

void star::RenderResourceSystem::bind(const BufferHandle& buffer, vk::CommandBuffer& commandBuffer)
{
	bindBuffer(buffer.getID(), commandBuffer, buffer.targetBufferOffset); 
}

void star::RenderResourceSystem::init(StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screensize)
{
	RenderResourceSystem::preparePrimaryGeometry(device);
	RenderResourceSystem::runInits(device, numFramesInFlight, screensize);
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
		std::function<void(const uint32_t&, const uint32_t&, const uint32_t&)>& offsetFunction = geometryDataOffsetCallbacks.top();
		std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>> result = function(device, vertBufferHandle, indBufferHandle);

		//might want to throw a check in here to verify buffer type
		if (!result.first || !result.second)
			throw std::runtime_error("Geometry data loading callback returned invalid buffers");

		{
			uint32_t numIndices = result.second->getInstanceCount();
			offsetFunction(totalVertInstanceCount, totalIndInstanceCount, numIndices);

			totalVertSize += result.first->getBufferSize();
			totalVertInstanceCount += result.first->getInstanceCount();
			totalIndSize += result.second->getInstanceSize();
			totalIndInstanceCount += result.second->getInstanceCount();
			stagingBuffersVert.push_back(std::move(result.first));
			stagingBuffersIndex.push_back(std::move(result.second));
		}


		loadGeometryCallbacks.pop();
		geometryDataOffsetCallbacks.pop();
	}

	vk::DeviceSize vertexSize = sizeof(star::Vertex);
	std::unique_ptr<StarBuffer> vertBuffer = std::make_unique<StarBuffer>(
		device.getAllocator().get(),
		vertexSize,
		uint32_t(totalVertInstanceCount),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::SharingMode::eConcurrent
	);
	{
		vk::DeviceSize currentOffset = 0;
		for (auto& vertStage : stagingBuffersVert) {
			device.copyBuffer(vertStage->getVulkanBuffer(), vertBuffer->getVulkanBuffer(), vertStage->getBufferSize(), currentOffset);
			currentOffset += vertStage->getBufferSize();
		}
	}

	vk::DeviceSize indSize = sizeof(uint32_t);
	std::unique_ptr<StarBuffer> indBuffer = std::make_unique<StarBuffer>(
		device.getAllocator().get(),
		indSize,
		uint32_t(totalIndInstanceCount),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::SharingMode::eConcurrent,
		1
	);

	{
		vk::DeviceSize currentOffset = 0; 

		for (auto& indStage : stagingBuffersIndex) {
			device.copyBuffer(indStage->getVulkanBuffer(), indBuffer->getVulkanBuffer(), indStage->getBufferSize(), currentOffset);

			currentOffset += indStage->getBufferSize(); 
		}
	}

	buffers.push_back(std::move(vertBuffer));
	buffers.push_back(std::move(indBuffer));
}

void star::RenderResourceSystem::runInits(StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screensize)
{
	while (!initCallbacks.empty()) {
		std::function<void(StarDevice&, const int&, const vk::Extent2D&)>& function = initCallbacks.top();
		function(device, numFramesInFlight, screensize); 
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
		commandBuffer.bindVertexBuffers(0, buffer.getVulkanBuffer(), vOffset);
	}
	else if (buffer.getUsageFlags() & vk::BufferUsageFlagBits::eIndexBuffer)
		commandBuffer.bindIndexBuffer(buffer.getVulkanBuffer(), vOffset, vk::IndexType::eUint32);
	else
		throw std::runtime_error("Unsupported buffer type requested for bind operation from Resource System");
}
