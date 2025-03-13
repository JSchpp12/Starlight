#pragma once

#include "Interactivity.hpp"
#include "ManagerRenderResource.hpp"
#include "Handle.hpp"
#include "StarObject.hpp"
#include "Light.hpp"
#include "StarWindow.hpp"
#include "StarRenderObject.hpp"
#include "BasicCamera.hpp"

#include <map>
#include <memory>
#include <vector>
#include <optional>

namespace star {
	/// <summary>
	/// Container for all objects in a scene. 
	/// </summary>
	class StarScene : public Interactivity {
	public:
		StarScene(const int& numFramesInFlight);

		StarScene(const int& numFramesInFlight, StarCamera* camera, std::vector<Handle> globalInfoBuffers);

		virtual ~StarScene() = default;

		int add(std::unique_ptr<Light> newLight);

		int add(std::unique_ptr<StarObject> newObject);

		StarCamera* getCamera() { 
			if (this->myCamera.has_value()) {
				return this->myCamera.value().get();
			}
			else {
				assert(this->externalCamera.has_value());
				assert(this->externalCamera.value() && "The external camera must be valid throughout the lifetime of this obejct");
				return this->externalCamera.value();
			}
		}

		StarObject& getObject(int objHandle) {
			return *this->objects.at(objHandle);
		}

		Light& getLight(int light) {
			return *this->lightList.at(light); 
		}

		std::vector<std::unique_ptr<Light>>& getLights() { return this->lightList; }
		std::vector<std::reference_wrapper<StarObject>> getObjects(); 
		Handle getGlobalInfoBuffer(const int& index) {return this->globalInfoBuffers.at(index); }
		Handle getLightInfoBuffer(const int& index) { return this->lightInfoBuffers.at(index); }
	protected:
		std::optional<StarCamera*> externalCamera = std::nullopt;
		std::optional<std::unique_ptr<StarCamera>> myCamera = std::nullopt; 

		std::vector<Handle> globalInfoBuffers = std::vector<Handle>(); 
		std::vector<Handle> lightInfoBuffers = std::vector<Handle>();

		int objCounter = 0; 
		int rObjCounter = 0; 
		int lightCounter = 0;  

		std::unordered_map<int, std::unique_ptr<StarObject>> objects = std::unordered_map<int, std::unique_ptr<StarObject>>(); 
		std::vector<std::unique_ptr<Light>> lightList = std::vector<std::unique_ptr<Light>>();

		virtual void onWorldUpdate(const uint32_t& frameInFlightIndex) override; 
	};
}