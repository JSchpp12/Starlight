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

    virtual void prepare() = 0;

    virtual void submit() = 0; 

protected:
    StarCamera& camera; 
    StarWindow& window;
    StarDevice& device;

	std::optional<std::reference_wrapper<StarRenderer>> waitFor; 

	StarRenderer(StarWindow& window, StarCamera& inCamera, StarDevice& device) 
        : window(window), camera(inCamera), device(device){};

	/// <summary>
	/// A renderer may request additional device extensions. Default is none.
	/// </summary>
	/// <returns></returns>
	std::vector<const char*> getRequiredDeviceExtensions() {
		return std::vector<const char*>();
	}

private:
	//Flag which is stored to track if this renderer has been initialized and created on the gpu
	bool isCreated = false; 
};
}