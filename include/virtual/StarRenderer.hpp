#pragma once 

#include "StarDevice.hpp"
#include "StarWindow.hpp"

#include <memory>

#include <vulkan/vulkan.hpp>

namespace star {
class StarRenderer {
public:
	StarRenderer() {}; 

	StarRenderer(const StarRenderer&) = delete; 
	StarRenderer& operator=(const StarRenderer&) = delete; 

	virtual ~StarRenderer() {};

protected:

private: 

};
}