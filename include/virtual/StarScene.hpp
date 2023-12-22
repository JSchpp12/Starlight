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

		virtual int add(std::unique_ptr<Light> newLight) { 
			int lightIndex = this->lightList.size();
			this->lightList.emplace_back(std::move(newLight));
			return lightIndex; 
		}

		virtual int add(std::unique_ptr<StarObject> newObject) { 
			int objHandle = objCounter; 
			this->objects.insert(std::pair<int, std::unique_ptr<StarObject>>(objCounter, std::move(newObject)));
			this->objCounter++; 
			return objHandle; 
		}

		StarObject& getObject(int objHandle) {
			return *this->objects.at(objHandle);
		}

		Light& getLight(int light) {
			return *this->lightList.at(light); 
		}

		virtual std::vector<std::unique_ptr<Light>>& getLights() { return this->lightList; }
		virtual std::unordered_map<int, std::unique_ptr<StarObject>>& getObjects() { return this->objects; }
	protected:
		int objCounter = 0; 
		int rObjCounter = 0; 
		//std::vector<std::unique_ptr<StarObject>> objectList;
		std::unordered_map<int, std::unique_ptr<StarObject>> objects; 
		std::vector<std::unique_ptr<Light>> lightList;
		

	};
}