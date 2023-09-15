#include "MaterialManager.hpp"

namespace star {
	Handle MaterialManager::add(std::unique_ptr<Material> newMaterial) {
		Handle newHandle;
		newHandle.type = Handle_Type::material;

		this->addResource(std::move(newMaterial));
		return newHandle;
	}

	Handle MaterialManager::add(const glm::vec4& surfaceColor, const glm::vec4& highlightColor, const glm::vec4& ambient,
		const glm::vec4& diffuse, const glm::vec4& specular, const int& shinyCoefficient) {
		Handle newHandle;
		newHandle.type = Handle_Type::material;

		std::unique_ptr<Material> newMaterial(new Material(surfaceColor, highlightColor, ambient, diffuse, specular, shinyCoefficient));
		this->addResource(std::move(newMaterial));
		return newHandle;
	}
	Handle MaterialManager::add(const glm::vec4& surfaceColor, const glm::vec4& highlightColor, const glm::vec4& ambient,
		const glm::vec4& diffuse, const glm::vec4& specular,
		const int& shinyCoefficient, Handle texture, Handle bumpMap)
	{
		return this->addResource(std::make_unique<Material>(surfaceColor, highlightColor, ambient, diffuse, specular, shinyCoefficient, texture, bumpMap));
	}

	Handle MaterialManager::createAppropriateHandle() {
		Handle newHandle;
		newHandle.type = Handle_Type::material;
		return newHandle;
	}
}