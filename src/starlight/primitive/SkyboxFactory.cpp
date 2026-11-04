#include "starlight/primitive/SkyboxFactory.hpp"

namespace star::primitive
{
CubeObject::OptionalRenderingOverrides CreateSkyboxRenderOverrides()
{
    // Explicitly sized depth state: depth test on, writes off, far-passing compare.
    // The remaining vk::PipelineDepthStencilStateCreateInfo fields (depth bounds
    // test, stencil) default to disabled in a default-constructed handle struct.
    vk::PipelineDepthStencilStateCreateInfo depthStencil = vk::PipelineDepthStencilStateCreateInfo()
                                                               .setDepthTestEnable(VK_TRUE)
                                                               .setDepthWriteEnable(VK_FALSE)
                                                               .setDepthCompareOp(vk::CompareOp::eLessOrEqual);

    return CubeObject::OptionalRenderingOverrides{.cullMode = vk::CullModeFlagBits::eNone,
                                                  .depthStencil = depthStencil};
}
} // namespace star::primitive