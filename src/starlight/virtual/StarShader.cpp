#include "StarShader.hpp"

#include <stdexcept>

namespace star
{

static vk::ShaderStageFlagBits ToVulkanStage(star::Shader_Stage stage)
{
    switch (stage)
    {
    case star::Shader_Stage::vertex:
        return vk::ShaderStageFlagBits::eVertex;
    case star::Shader_Stage::fragment:
        return vk::ShaderStageFlagBits::eFragment;
    case star::Shader_Stage::compute:
        return vk::ShaderStageFlagBits::eCompute;
    case star::Shader_Stage::geometry:
        return vk::ShaderStageFlagBits::eGeometry;
    default:
        throw std::runtime_error("Unsupported shader stage");
    }
}

StarShader::StarShader(vk::Device device, star::Shader_Stage stage, const std::vector<uint32_t> &compiledCode)
    : stage(stage)
{
    createModule(device, compiledCode);
}

StarShader::StarShader(StarShader &&other) noexcept : m_module(other.m_module), stage(other.stage)
{
    other.m_module = VK_NULL_HANDLE;
    other.stage = star::Shader_Stage::none;
}

StarShader &StarShader::operator=(StarShader &&other) noexcept
{
    if (this != &other)
    {
        m_module = other.m_module;
        stage = other.stage;

        other.m_module = VK_NULL_HANDLE;
        other.stage = star::Shader_Stage::none;
    }

    return *this;
}

void StarShader::createModule(vk::Device device, const std::vector<uint32_t> &compiledCode)
{
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = compiledCode.size() * sizeof(uint32_t);
    createInfo.pCode = compiledCode.data();

    m_module = device.createShaderModule(createInfo);
    if (!m_module)
    {
        throw std::runtime_error("failed to create shader module");
    }
}

vk::PipelineShaderStageCreateInfo StarShader::getStageCreateInfo() const
{
    vk::PipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    stageInfo.stage = ToVulkanStage(stage);
    stageInfo.module = m_module;
    stageInfo.pName = "main";

    return stageInfo;
}

void StarShader::cleanupRender(vk::Device device)
{
    if (m_module)
    {
        device.destroyShaderModule(m_module);
        m_module = VK_NULL_HANDLE;
    }
}
} // namespace star
