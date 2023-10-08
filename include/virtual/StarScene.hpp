#pragma once
#include "Handle.hpp"
#include "StarObject.hpp"
#include "Light.hpp"

#include "Camera.hpp"

#include <vector>
#include <memory>

namespace star {
	/// <summary>
	/// Container for all objects in a scene. 
	/// </summary>
	class StarScene {
	public:
		StarScene() = default;
		virtual ~StarScene() = default;

		virtual void add(std::unique_ptr<Light> newLight) { this->lightList.emplace_back(std::move(newLight));}
		virtual void add(std::unique_ptr<StarObject> newObject) { this->objectList.emplace_back(std::move(newObject)); }
		virtual std::vector<std::unique_ptr<Light>>& getLights() { return this->lightList; }
		virtual std::vector<std::unique_ptr<StarObject>>& getObjects() { return this->objectList; }
	protected:
		std::vector<std::unique_ptr<StarObject>> objectList;
		std::vector<std::unique_ptr<Light>> lightList;

	};
}