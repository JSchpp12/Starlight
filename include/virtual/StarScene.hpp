#pragma once
#include "Handle.hpp"
#include "StarObject.hpp"
#include "Light.hpp"
#include "StarWindow.hpp"
#include "StarRenderObject.hpp"

#include "StarCamera.hpp"

#include <map>
#include <memory>

namespace star {
	/// <summary>
	/// Container for all objects in a scene. 
	/// </summary>
	class StarScene {
	public:
		StarScene() = default;
		virtual ~StarScene() = default;

		int add(std::unique_ptr<Light> newLight);

		int add(std::unique_ptr<StarObject> newObject);

		StarObject& getObject(int objHandle) {
			return *this->objects.at(objHandle);
		}

		Light& getLight(int light) {
			return *this->lightList.at(light); 
		}

		std::vector<std::unique_ptr<Light>>& getLights() { return this->lightList; }
		std::vector<std::reference_wrapper<StarObject>> getObjects(); 
	protected:
		int objCounter = 0; 
		int rObjCounter = 0; 
		//std::vector<std::unique_ptr<StarObject>> objectList;
		std::unordered_map<int, std::unique_ptr<StarObject>> objects = std::unordered_map<int, std::unique_ptr<StarObject>>(); 
		std::vector<std::unique_ptr<Light>> lightList = std::vector<std::unique_ptr<Light>>();
		

	};
}