#pragma once

#include "FileResourceManager.hpp"
#include "StarCamera.hpp"
#include "device/StarDevice.hpp"
#include "StarShader.hpp"
#include "StarWindow.hpp"

#include <vulkan/vulkan.hpp>
#include <memory>
#include <optional>

namespace star
{
/// <summary>
/// Objects which share a framebuffer can share the same star renderer
/// </summary>
class StarRenderer
{
  public:
    virtual ~StarRenderer() = default;
    StarRenderer(const StarRenderer &) = delete;
    StarRenderer &operator=(const StarRenderer &) = delete;

  protected:
    const std::shared_ptr<StarCamera> camera = nullptr;

    explicit StarRenderer(const std::shared_ptr<StarCamera> camera) : camera(camera) {};
};
} // namespace star