#pragma once

#include "core/DeviceContext.hpp"

#include "vulkan/vulkan.hpp"

#include <string>
#include <vector>


namespace star
{
class StarPipeline
{
  public:
    StarPipeline() = default;
    virtual ~StarPipeline();
    StarPipeline(const StarPipeline &) = delete;
    StarPipeline &operator=(const StarPipeline &) = delete;
    StarPipeline(StarPipeline &&other)
    {
        m_pipeline = other.m_pipeline;
        m_hash = other.m_hash;

        other.m_pipeline = VK_NULL_HANDLE;
    }
    StarPipeline &operator=(StarPipeline &&other)
    {
        if (this != &other)
        {
            m_pipeline = other.m_pipeline;
            m_hash = other.m_hash;

            other.m_pipeline = VK_NULL_HANDLE;
        }

		return *this;
    }
    
    bool isRenderReady(core::device::DeviceContext& device); 

    void init(core::device::DeviceContext &context);

    void prepRender(core::device::DeviceContext &context); 

    void cleanup(core::device::DeviceContext &context);

    virtual void bind(vk::CommandBuffer &commandBuffer) = 0;

    const std::vector<Handle> &getShaders(){
        return m_shaders; 
    }

  protected:
    vk::Pipeline m_pipeline;
    std::string m_hash; // this is simply the paths of all shaders in this pipeline concated together

    virtual vk::Pipeline buildPipeline(core::device::DeviceContext &context) = 0;

	///Child objects should submit requests to the shader manager in this function call and provide the handles
	virtual std::vector<Handle> submitShaders(core::device::DeviceContext &context)=0;

    bool isSame(StarPipeline &compPipe);

#pragma region helpers
    /// <summary>
    /// Build a shader module with the provided SPIR-V source code
    /// </summary>
    /// <param name="sourceCode"></param>
    /// <returns></returns>
    vk::ShaderModule createShaderModule(core::device::StarDevice &device, const std::vector<uint32_t> &sourceCode);
#pragma endregion

private:
std::vector<Handle> m_shaders; 
};
} // namespace star