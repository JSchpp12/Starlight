#pragma once 

#include "StarDevice.hpp"
#include "StarWindow.hpp"
#include "StarShader.hpp"
#include "FileResourceManager.hpp"
#include "StarCamera.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>

namespace star {
/// <summary>
/// Objects which share a framebuffer can share the same star renderer
/// </summary>
class StarRenderer {
public:
	virtual ~StarRenderer() = default; 
	StarRenderer(const StarRenderer&) = delete; 
	StarRenderer& operator=(const StarRenderer&) = delete; 

    //virtual void prepare() = 0;

protected:
    const StarCamera* camera; 

	std::optional<std::reference_wrapper<StarRenderer>> waitFor; 

	StarRenderer(const StarCamera* inCamera) 
        : camera(inCamera){};
};
}