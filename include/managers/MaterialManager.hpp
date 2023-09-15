#pragma once 
#include "StarMemoryManager.hpp"
#include "Material.hpp"
#include "Enums.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace star {
class MaterialManager : public StarMemoryManager<Material> {
public:
	MaterialManager(std::unique_ptr<Material> defaultMaterial) : StarMemoryManager<Material>() {
		this->StarMemoryManager<Material>::init(std::move(defaultMaterial));
	}

	Handle add(std::unique_ptr<Material> newMaterial);

	Handle add(const glm::vec4& surfaceColor, const glm::vec4& highlightColor, const glm::vec4& ambient,
		const glm::vec4& diffuse, const glm::vec4& specular, const int& shinyCoefficient);

	Handle add(const glm::vec4& surfaceColor, const glm::vec4& highlightColor, const glm::vec4& ambient,
		const glm::vec4& diffuse, const glm::vec4& specular,
		const int& shinyCoefficient, Handle texture,
		Handle bumpMap);

	size_t size() { return this->StarMemoryManager::size(); }

protected:
	virtual Handle createAppropriateHandle() override;

private:

};
}