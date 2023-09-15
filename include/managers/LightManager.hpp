#pragma once

#include "Light.hpp"
#include "Enums.hpp"
#include "Handle.hpp"
#include "StarMemoryManager.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace star {
	class LightManager : public StarMemoryManager<Light> {
	public:
		Handle add(std::unique_ptr<Light> newLight);

	protected:
		virtual Handle createAppropriateHandle() override;

	private:
		std::vector<std::unique_ptr<Light>> lightContainer;

	};
}