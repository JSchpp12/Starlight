#pragma once

#include "Enums.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star
{
class StarShader
{
  public:
    StarShader() = default;
    explicit StarShader(star::Shader_Stage stage) : stage(stage)
    {
    }

    /// Builds the shader module from compiled SPIR-V. Called from the
    /// CompileShader job.
    StarShader(vk::Device device, star::Shader_Stage stage, const std::vector<uint32_t> &compiledCode);

    StarShader(const StarShader &) = delete;
    StarShader &operator=(const StarShader &) = delete;
    StarShader(StarShader &&other) noexcept;
    StarShader &operator=(StarShader &&other) noexcept;
    ~StarShader() = default;

    bool isValid() const
    {
        return m_module != VK_NULL_HANDLE;
    }

    star::Shader_Stage getStage() const
    {
        return this->stage;
    }

    vk::ShaderModule getModule() const
    {
        return m_module;
    }

    vk::PipelineShaderStageCreateInfo getStageCreateInfo() const;

    void cleanupRender(vk::Device device);

  private:
    void createModule(vk::Device device, const std::vector<uint32_t> &compiledCode);

    vk::ShaderModule m_module = VK_NULL_HANDLE;
    star::Shader_Stage stage = star::Shader_Stage::none;
};
} // namespace star
