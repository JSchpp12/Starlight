#pragma once

#include "enums/Enums.hpp"

#include <vulkan/vulkan.hpp>

namespace star::core::renderer
{
struct RenderPhaseConfig
{
    Queue_Type queueType = Queue_Type::Tgraphics;
    Command_Buffer_Order order = Command_Buffer_Order::main_render_pass;
    Command_Buffer_Order_Index orderIndex = star::Command_Buffer_Order_Index::first;
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eFragmentShader;
    bool willBeSubmittedEachFrame = true;
    bool recordOnce = false;
};
} // namespace star::core::renderer