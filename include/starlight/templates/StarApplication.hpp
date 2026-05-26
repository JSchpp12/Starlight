#pragma once

#include "StarScene.hpp"
#include "core/SystemContext.hpp"

#include <memory>

namespace star
{
	class StarApplication
	{
	public:
		virtual ~StarApplication() = default;
		virtual void init() = 0;
		virtual void frameUpdate(core::SystemContext& systemContext) = 0;
		virtual void shutdown(core::device::DeviceContext& context) = 0;
		virtual std::shared_ptr<StarScene> loadScene(core::device::DeviceContext& context) = 0;
	};
} // namespace star
