#include "StarObject.hpp"

#include "ManagerController_RenderResource_InstanceModelInfo.hpp"
#include "ManagerController_RenderResource_InstanceNormalInfo.hpp"
#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"

#include <algorithm>

std::unique_ptr<star::StarDescriptorSetLayout> star::StarObject::instanceDescriptorLayout =
    std::unique_ptr<star::StarDescriptorSetLayout>();
vk::PipelineLayout star::StarObject::extrusionPipelineLayout = vk::PipelineLayout{};
std::unique_ptr<star::Handle> star::StarObject::tri_normalExtrusionPipeline = std::unique_ptr<star::Handle>();
std::unique_ptr<star::Handle> star::StarObject::triAdj_normalExtrusionPipeline = std::unique_ptr<star::Handle>();
std::unique_ptr<star::StarDescriptorSetLayout> star::StarObject::boundDescriptorLayout =
    std::unique_ptr<star::StarDescriptorSetLayout>();
vk::PipelineLayout star::StarObject::boundPipelineLayout = vk::PipelineLayout{};
std::unique_ptr<star::Handle> star::StarObject::boundBoxPipeline = std::unique_ptr<star::Handle>();

void star::StarObject::initSharedResources(core::device::DeviceContext &device, vk::Extent2D swapChainExtent,
                                           int numSwapChainImages, StarDescriptorSetLayout &globalDescriptors,
                                           core::renderer::RenderingTargetInfo renderingInfo)
{
    // std::string mediaPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    // instanceDescriptorLayout = StarDescriptorSetLayout::Builder(device.getDevice())
    //                                .addBinding(0, vk::DescriptorType::eUniformBuffer,
    //                                vk::ShaderStageFlagBits::eVertex) .addBinding(1,
    //                                vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex) .build();

    // // this must match the descriptors for the StarObjectInstance
    // {
    //     std::vector<vk::DescriptorSetLayout> globalLayouts{globalDescriptors.getDescriptorSetLayout(),
    //                                                        instanceDescriptorLayout->getDescriptorSetLayout()};

    //     vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    //     pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    //     pipelineLayoutInfo.setLayoutCount = globalLayouts.size();
    //     pipelineLayoutInfo.pSetLayouts = globalLayouts.data();
    //     pipelineLayoutInfo.pPushConstantRanges = nullptr;
    //     pipelineLayoutInfo.pushConstantRangeCount = 0;
    //     extrusionPipelineLayout = device.getDevice().getVulkanDevice().createPipelineLayout(pipelineLayoutInfo);
    // }
    // {
    //     std::string vertPath = mediaPath + "/shaders/extrudeNormals/extrudeNormals.vert";
    //     std::string fragPath = mediaPath + "/shaders/extrudeNormals/extrudeNormals.frag";

    //     StarShader vert = StarShader(vertPath, Shader_Stage::vertex);
    //     StarShader frag = StarShader(fragPath, Shader_Stage::fragment);

    //     // StarGraphicsPipeline::PipelineConfigSettings settings;
    //     // StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, extrusionPipelineLayout,
    //     //                                                 renderingInfo);
    //     {
    //         std::string geomPath = mediaPath + "shaders/extrudeNormals/extrudeNormals_triangle.geom";
    //         StarShader geom = StarShader(geomPath, Shader_Stage::geometry);

    //         // StarObject::tri_normalExtrusionPipeline = std::make_unique<StarGraphicsPipeline>(vert, frag, geom);
    //         StarObject::tri_normalExtrusionPipeline = std::make_unique<Handle>(device.getShaderManager(
    //             core::device::manager::PipelineRequest(swapChainExtent,renderingInfo,
    //                 StarPipeline(StarPipeline::GraphicsPipelineConfigSettings(), extrusionPipelineLayout,
    //             std::vector<Handle>{})
    //         ))
    //         StarObject::tri_normalExtrusionPipeline->init(device, extrusionPipelineLayout);
    //     }
    //     {
    //         settings.inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleListWithAdjacency;

    //         std::string geomPath = mediaPath + "shaders/extrudeNormals/extrudeNormals_triangleAdj.geom";
    //         StarShader geom = StarShader(geomPath, Shader_Stage::geometry);

    //         StarObject::triAdj_normalExtrusionPipeline = std::make_unique<StarGraphicsPipeline>(vert, frag, geom);
    //         StarObject::triAdj_normalExtrusionPipeline->init(device, extrusionPipelineLayout);
    //     }
    // }

    // boundDescriptorLayout = StarDescriptorSetLayout::Builder(device.getDevice())
    //                             .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
    //                             .build();

    // {
    //     std::vector<vk::DescriptorSetLayout> globalLayouts{globalDescriptors.getDescriptorSetLayout(),
    //                                                        boundDescriptorLayout->getDescriptorSetLayout()};

    //     vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    //     pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    //     pipelineLayoutInfo.setLayoutCount = globalLayouts.size();
    //     pipelineLayoutInfo.pSetLayouts = globalLayouts.data();
    //     pipelineLayoutInfo.pPushConstantRanges = nullptr;
    //     pipelineLayoutInfo.pushConstantRangeCount = 0;
    //     boundPipelineLayout = device.getDevice().getVulkanDevice().createPipelineLayout(pipelineLayoutInfo);

    //     StarGraphicsPipeline::PipelineConfigSettings settings;
    //     StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, extrusionPipelineLayout,
    //                                                     renderingInfo);

    //     settings.inputAssemblyInfo.topology = vk::PrimitiveTopology::eLineList;

    //     // bounding box vert buffer will be bound to the 1st index
    //     std::string boundVertPath = mediaPath + "shaders/boundingBox/bounding.vert";
    //     std::string boundFragPath = mediaPath + "shaders/boundingBox/bounding.frag";
    //     StarShader vert = StarShader(boundVertPath, Shader_Stage::vertex);
    //     StarShader frag = StarShader(boundFragPath, Shader_Stage::fragment);

    //     StarObject::boundBoxPipeline = std::make_unique<StarGraphicsPipeline>(vert, frag);
    //     boundBoxPipeline->init(device, extrusionPipelineLayout);
    // }
}

void star::StarObject::cleanupSharedResources(core::device::DeviceContext &device)
{
    // instanceDescriptorLayout.reset();
    // device.getDevice().getVulkanDevice().destroyPipelineLayout(extrusionPipelineLayout);
    // StarObject::triAdj_normalExtrusionPipeline.reset();
    // StarObject::tri_normalExtrusionPipeline.reset();

    // boundDescriptorLayout.reset();
    // device.getDevice().getVulkanDevice().destroyPipelineLayout(boundPipelineLayout);
    // StarObject::boundBoxPipeline.reset();
}

void star::StarObject::cleanupRender(core::device::DeviceContext &context)
{
    // normalExtrusionPipeline->cleanupRender(context.getDevice());
    // this->normalExtrusionPipeline.reset();

    // this->setLayout.reset();

    // cleanup any material

    for (auto &material : m_meshMaterials)
    {
        material->cleanupRender(context);
    }
}

star::Handle star::StarObject::buildPipeline(core::device::DeviceContext &context, vk::Extent2D swapChainExtent,
                                             vk::PipelineLayout pipelineLayout,
                                             core::renderer::RenderingTargetInfo renderInfo)
{
    auto graphicsShaders = this->getShaders();

    return context.getPipelineManager().submit(core::device::manager::PipelineRequest(
        StarPipeline(
            StarPipeline::GraphicsPipelineConfigSettings(), pipelineLayout,
            std::vector<Handle>{context.getShaderManager().submit(graphicsShaders.at(Shader_Stage::vertex)),
                                context.getShaderManager().submit(graphicsShaders.at(Shader_Stage::fragment))}),
        swapChainExtent, renderInfo));
}

star::StarObjectInstance &star::StarObject::getInstance(const size_t &index)
{
    assert(instances.at(index));

    return *this->instances[index];
}

void star::StarObject::prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
                                  const uint8_t &numFramesInFlight, star::StarShaderInfo::Builder fullEngineBuilder,
                                  Handle sharedPipeline)
{
    this->sharedPipeline = sharedPipeline;

    prepStarObject(context, numFramesInFlight, fullEngineBuilder);
}

void star::StarObject::prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
                                  const uint8_t &numFramesInFlight, StarShaderInfo::Builder fullEngineBuilder,
                                  vk::PipelineLayout pipelineLayout, core::renderer::RenderingTargetInfo renderingInfo)
{
    this->pipeline = buildPipeline(context, swapChainExtent, pipelineLayout, renderingInfo);

    prepStarObject(context, numFramesInFlight, fullEngineBuilder);
}

void star::StarObject::prepStarObject(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                      StarShaderInfo::Builder &frameBuilder)
{
    createInstanceBuffers(context, numFramesInFlight);
    prepMaterials(context, numFramesInFlight, frameBuilder);

    this->meshes = loadMeshes(context);
    m_deviceID = context.getDeviceID();

    {
        std::vector<Vertex> bbVerts;
        std::vector<uint32_t> bbInds;

        calculateBoundingBox(bbVerts, bbInds);

        {
            auto bbSemaphore = context.getSemaphoreManager().submit(core::device::manager::SemaphoreRequest{false});
            this->boundingBoxVertBuffer = ManagerRenderResource::addRequest(
                m_deviceID, context.getSemaphoreManager().get(bbSemaphore)->semaphore,
                std::make_unique<TransferRequest::VertInfo>(
                    context.getDevice().getDefaultQueue(Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
                    std::move(bbVerts)));
        }
        {
            auto bbIndSemaphore = context.getSemaphoreManager().submit(core::device::manager::SemaphoreRequest{false});

            this->boundingBoxIndexBuffer = ManagerRenderResource::addRequest(
                m_deviceID, context.getSemaphoreManager().get(bbIndSemaphore)->semaphore,
                std::make_unique<TransferRequest::IndicesInfo>(
                    context.getDevice().getDefaultQueue(Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
                    std::move(bbInds)));
        }
    }

    prepareMeshes(context);

    renderingContext = std::make_unique<core::renderer::RenderingContext>(buildRenderingContext(context));
}

star::core::renderer::RenderingContext star::StarObject::buildRenderingContext(
    star::core::device::DeviceContext &context)
{
    return core::renderer::RenderingContext{context.getPipelineManager().get(pipeline)->request.pipeline};
}

void star::StarObject::recordRenderPassCommands(vk::CommandBuffer &commandBuffer, vk::PipelineLayout &pipelineLayout,
                                                uint8_t swapChainIndexNum)
{
    if (!isReady)
    {
        return;
    }

    for (auto &rmesh : this->meshes)
    {
        if (!rmesh->isKnownToBeReady(swapChainIndexNum))
        {
            return;
        }
    }

    renderingContext->pipeline.bind(commandBuffer);

    for (auto &rmesh : this->meshes)
    {
        uint32_t instanceCount = static_cast<uint32_t>(this->instances.size());
        rmesh.get()->recordRenderPassCommands(commandBuffer, pipelineLayout, swapChainIndexNum, instanceCount);
    }

    if (this->drawNormals)
        recordDrawCommandNormals(commandBuffer);
    if (this->drawBoundingBox)
        recordDrawCommandBoundingBox(commandBuffer, swapChainIndexNum);
}

star::StarObjectInstance &star::StarObject::createInstance()
{
    int instanceCount = static_cast<int>(this->instances.size());

    this->instances.push_back(std::make_unique<StarObjectInstance>(instanceCount));
    return *this->instances.back();
}

void star::StarObject::frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex, const Handle &targetCommandBuffer)
{
    if (!isReady && isRenderReady(context))
    {
        isReady = true;
    }

    updateInstanceData(context, frameInFlightIndex, targetCommandBuffer);
}

std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> star::StarObject::getDescriptorSetLayouts(
    core::device::DeviceContext &context)
{
    auto allSets = std::vector<std::shared_ptr<star::StarDescriptorSetLayout>>();
    auto staticSetBuilder = StarDescriptorSetLayout::Builder();

    StarDescriptorSetLayout::Builder updateSetBuilder =
        StarDescriptorSetLayout::Builder()
            .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
            .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
    allSets.emplace_back(updateSetBuilder.build());

    assert(m_meshMaterials.size() > 0 && "Materials should always exist");
    m_meshMaterials.front()->addDescriptorSetLayoutsTo(staticSetBuilder);
    auto staticSet = staticSetBuilder.build();

    if (staticSet->getBindings().size() > 0)
        allSets.push_back(std::move(staticSet));

    return allSets;
}

void star::StarObject::prepareMeshes(star::core::device::DeviceContext &device)
{
    assert(this->meshes.size() > 0 && "Meshes need to be provided");

    for (auto &mesh : this->meshes)
    {
        mesh->prepRender(device);
    }
}

void star::StarObject::prepMaterials(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                     star::StarShaderInfo::Builder &frameBuilder)
{
    assert(m_meshMaterials.size() > 0 && "Mesh materials should exist");

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        const auto &instanceModelHandle = m_infoManagerInstanceModel->getHandle(i);
        const auto &instanceNormalHandle = m_infoManagerInstanceNormal->getHandle(i);

        frameBuilder.startOnFrameIndex(i);
        frameBuilder.startSet();
        frameBuilder.add(instanceModelHandle, &context.getManagerRenderResource()
                                                   .get<StarBuffers::Buffer>(context.getDeviceID(), instanceModelHandle)
                                                   ->resourceSemaphore);
        frameBuilder.add(instanceNormalHandle,
                         &context.getManagerRenderResource()
                              .get<StarBuffers::Buffer>(context.getDeviceID(), instanceNormalHandle)
                              ->resourceSemaphore);
    }

    for (auto &material : m_meshMaterials)
    {
        // descriptors
        material->prepRender(context, numFramesInFlight, frameBuilder);
    }
}

void star::StarObject::createInstanceBuffers(star::core::device::DeviceContext &context,
                                             const uint8_t &numFramesInFlight)
{
    assert(this->instances.size() > 0 &&
           "Call to create instance buffers made but this object does not have any instances");
    assert(this->instances.size() < 1024 && "Max number of supported instances is 1024");

    m_infoManagerInstanceModel =
        std::make_unique<ManagerController::RenderResource::InstanceModelInfo>(numFramesInFlight, this->instances);
    m_infoManagerInstanceModel->prepRender(context, numFramesInFlight);

    m_infoManagerInstanceNormal =
        std::make_unique<ManagerController::RenderResource::InstanceNormalInfo>(numFramesInFlight, this->instances);
    m_infoManagerInstanceNormal->prepRender(context, numFramesInFlight);
}

void star::StarObject::createBoundingBox(std::vector<Vertex> &verts, std::vector<uint32_t> &inds)
{
    std::array<glm::vec3, 2> bbBounds = this->meshes.front()->getBoundingBoxCoords();

    for (size_t i = 1; i < this->meshes.size(); i++)
    {
        std::array<glm::vec3, 2> curbbBounds = this->meshes.at(i)->getBoundingBoxCoords();

        if (curbbBounds[0].x < bbBounds[0].x)
            bbBounds[0].x = curbbBounds[0].x;
        if (curbbBounds[0].y < bbBounds[0].y)
            bbBounds[0].y = curbbBounds[0].y;
        if (curbbBounds[0].z < curbbBounds[0].z)
            bbBounds[0].z = curbbBounds[0].z;

        if (curbbBounds[1].x > bbBounds[1].x)
            bbBounds[1].x = curbbBounds[1].x;
        if (curbbBounds[1].y > bbBounds[1].y)
            bbBounds[1].y = curbbBounds[1].y;
        if (curbbBounds[1].z > bbBounds[1].z)
            bbBounds[1].z = curbbBounds[1].z;
    }

    star::GeometryHelpers::calculateAxisAlignedBoundingBox(bbBounds[0], bbBounds[1], verts, inds, true);
}

void star::StarObject::recordDrawCommandNormals(vk::CommandBuffer &commandBuffer)
{
    // uint32_t ib = 0;

    // // assuming all meshes have same packing approach at this point. Should have checked earlier on during load time
    // bool useAdjPipe = this->getMeshes().front()->hasAdjacentVertsPacked();

    // if (useAdjPipe)
    //     StarObject::triAdj_normalExtrusionPipeline->bind(commandBuffer);
    // else
    //     StarObject::tri_normalExtrusionPipeline->bind(commandBuffer);

    // commandBuffer.setLineWidth(1.0f);

    // for (auto &rmesh : this->getMeshes())
    // {
    //     uint32_t instanceCount = static_cast<uint32_t>(this->instances.size());
    //     uint32_t indexCount = rmesh->getNumIndices();
    //     commandBuffer.drawIndexed(indexCount, instanceCount, ib, 0, 0);

    //     ib += rmesh->getNumIndices();
    // }
}

void star::StarObject::recordDrawCommandBoundingBox(vk::CommandBuffer &commandBuffer, int inFlightIndex)
{
    // vk::DeviceSize offsets{};
    // commandBuffer.bindVertexBuffers(
    //     0, ManagerRenderResource::getBuffer(m_deviceID, this->boundingBoxVertBuffer).getVulkanBuffer(), offsets);
    // commandBuffer.bindIndexBuffer(
    //     ManagerRenderResource::getBuffer(m_deviceID, this->boundingBoxIndexBuffer).getVulkanBuffer(), 0,
    //     vk::IndexType::eUint32);

    // this->boundBoxPipeline->bind(commandBuffer);
    // commandBuffer.setLineWidth(1.0f);

    // commandBuffer.drawIndexed(this->boundingBoxIndsCount, 1, 0, 0, 0);
}

void star::StarObject::calculateBoundingBox(std::vector<Vertex> &verts, std::vector<uint32_t> &inds)
{
    assert(this->meshes.size() > 0 && "This function must be called after meshes are loaded");

    this->createBoundingBox(verts, inds);
    this->boundingBoxIndsCount = inds.size();
}

std::vector<std::pair<vk::DescriptorType, const int>> star::StarObject::getDescriptorRequests(
    const int &numFramesInFlight)
{
    std::vector<std::pair<vk::DescriptorType, const int>> requests{
        std::make_pair(vk::DescriptorType::eUniformBuffer, 2)};

    for (auto &material : m_meshMaterials)
    {
        auto matRequests = material->getDescriptorRequests(numFramesInFlight);
        for (auto &type : matRequests)
        {
            requests.push_back(std::move(type));
        }
    }

    return requests;
}

bool star::StarObject::isRenderReady(core::device::DeviceContext &context)
{
    assert(this->meshes.size() > 0 &&
           "Meshes have not yet been prepared. Need to have been created in the constructors");

    return context.getPipelineManager().get(this->pipeline)->isReady();
}

std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>> star::StarObject::submitDataUpdates(core::device::DeviceContext &context,
    const uint8_t &frameInFlightIndex) const
{
    auto semaphoreInfo = std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>>();

    for (const auto &material : m_meshMaterials)
    {
        const auto currentSemaphores = std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>>(semaphoreInfo);

        const auto materialSemaphores = material->getDataSemaphores(frameInFlightIndex);

        std::set_union(currentSemaphores.begin(), currentSemaphores.end(), materialSemaphores.begin(),
                       materialSemaphores.end(), std::inserter(semaphoreInfo, semaphoreInfo.begin()));
    }

    return semaphoreInfo;
}

void star::StarObject::updateInstanceData(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex, const Handle &targetCommandBuffer){
    CommandBufferContainer::CompleteRequest &request = context.getManagerCommandBuffer().m_manager.get(targetCommandBuffer);
    vk::Semaphore doneSemaphore; 

    if (m_infoManagerInstanceModel->submitUpdateIfNeeded(context, frameInFlightIndex, doneSemaphore)){
        request.oneTimeWaitSemaphores.insert(std::make_pair(std::move(doneSemaphore), vk::PipelineStageFlagBits::eVertexShader));
    }

    if (m_infoManagerInstanceNormal->submitUpdateIfNeeded(context, frameInFlightIndex, doneSemaphore)){
        request.oneTimeWaitSemaphores.insert(std::make_pair(std::move(doneSemaphore), vk::PipelineStageFlagBits::eFragmentShader)); 
    }
}