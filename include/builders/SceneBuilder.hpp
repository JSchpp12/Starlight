//#pragma once 
//
//#include "FileHelpers.hpp"
//#include "Light.hpp"
//#include "Handle.hpp"
//#include "BumpMaterial.hpp"
//#include "StarObject.hpp"
//#include "Triangle.hpp"
//#include "Vertex.hpp"
//
//#include "ObjectManager.hpp"
//#include "LightManager.hpp"
//#include "TextureManager.hpp"
//#include "MapManager.hpp"
//
//#include <glm/glm.hpp>
//#include <tiny_obj_loader.h>
//
//#include <memory>
//#include <string>
//#include <vector>
//
//namespace star {
//	class SceneBuilder {
//	public:
//		class GameObjects {
//		public:
//			class Builder {
//			public:
//				struct OverrideMaterialProperties {
//					const Handle* baseColorTexture = nullptr;
//					const glm::vec3* ambient = nullptr;
//					const glm::vec3* diffuse = nullptr;
//					const glm::vec3* specular = nullptr;
//					const float* shiny = nullptr;
//				};
//
//				Builder(SceneBuilder& sceneBuilder) : sceneBuilder(sceneBuilder) {
//					vertShader.shaderStage = Shader_Stage::vertex;
//					fragShader.shaderStage = Shader_Stage::fragment;
//				};
//				Builder& setPosition(const glm::vec3 position);
//				//override vertex colors from file with a predefined one
//				Builder& setColor(const glm::vec4& color);
//				Builder& setPath(const std::string& path);
//				Builder& setScale(const glm::vec3 scale);
//				Builder& setVertShader(const Handle& vertShader);
//				Builder& setFragShader(const Handle& fragShader);
//				Builder& setTexture(const Handle& texture);
//				Builder& setVerticies(const std::vector<glm::vec3>& verticies);
//				Builder& setIndicies(const std::vector<uint32_t>& indicies);
//				Builder& setMaterial(Handle materialHandle);
//				Builder& overrideAmbient(const glm::vec3& ambient);
//				Builder& overrideDiffuse(const glm::vec3& diffuse);
//				Builder& overrideSpecular(const glm::vec3& specular);
//				Builder& overrideShiny(const float& shiny);
//				Builder& setMaterialFilePath(const std::string& path);
//				Builder& setTextureDirectory(const std::string& path);
//				Handle build(bool loadMaterials = true);
//				GameObject& buildGet(bool loadMaterials = true);
//
//			protected:
//
//			private:
//				SceneBuilder& sceneBuilder;
//				std::unique_ptr<OverrideMaterialProperties> matOverride;
//				bool loadFromDisk = true;
//				const glm::vec4* color = nullptr;
//				glm::vec3 scale = glm::vec3{ 1.0f, 1.0f, 1.0f };
//				glm::vec3 position = glm::vec3{ 0.0f, 0.0f, 0.0f };
//				Handle vertShader = Handle::getDefault();
//				Handle fragShader = Handle::getDefault();
//				Handle texture = Handle::getDefault();
//				Handle* materialHandle = nullptr;
//				std::unique_ptr<std::string> path;
//				std::unique_ptr<std::string> materialFilePath;
//				std::unique_ptr<std::string> textureDirectory;
//				std::unique_ptr<std::vector<uint32_t>> indicies;
//				std::unique_ptr<std::vector<Vertex>> verticies;
//			};
//
//		private:
//
//
//		};
//
//		class Lights {
//		public:
//			class Builder {
//			public:
//				Builder(SceneBuilder& sceneBuilder) : sceneBuilder(sceneBuilder) { }
//				Builder& setType(const Type::Light& type);
//				Builder& setLinkedObject(const Handle& linkedObject);
//				Builder& setPosition(const glm::vec3& position);
//				Builder& setAmbient(const glm::vec4& ambient);
//				Builder& setDiffuse(const glm::vec4& diffuse);
//				Builder& setSpecular(const glm::vec4& position);
//				//Set the direction wheree the light is pointing towards
//				Builder& setDirection(const glm::vec4& direction);
//				Builder& setDiameter(const float& innerDiameter, const float& outerDiameter);
//				Handle build();
//
//			private:
//				SceneBuilder& sceneBuilder;
//				const Handle* linkedHandle = nullptr;
//				const Type::Light* type = nullptr;
//				const glm::vec4* lightDirection = nullptr;
//				const glm::vec3* position = nullptr;
//				const glm::vec4* ambient = nullptr;
//				const glm::vec4* diffuse = nullptr;
//				const glm::vec4* specular = nullptr;
//				const float* innerDiameter = nullptr;
//				const float* outerDiameter = nullptr;
//			};
//		private:
//
//
//		};
//
//		std::vector<Light> lightList;
//
//		SceneBuilder(ObjectManager& objectManager,
//			MapManager& mapManager, LightManager& lightManager)
//			: objectManager(objectManager),
//			mapManager(mapManager), lightManager(lightManager) { }
//		~SceneBuilder() = default;
//
//		//todo: currently this only returns game objects, see if there is way to expand this
//		GameObject& entity(const Handle& handle);
//		Light& light(const Handle& handle);
//
//	private:
//		ObjectManager& objectManager;
//		LightManager& lightManager;
//		MapManager& mapManager;
//
//		Handle addObject(const std::string& pathToFile, glm::vec3& position, glm::vec3& scaleAmt,
//			Handle* materialHandle, Handle& vertShader,
//			Handle& fragShader, bool loadMaterials,
//			std::string* materialFilePath, std::string* textureDir,
//			const glm::vec4* overrideColor = nullptr, const GameObjects::Builder::OverrideMaterialProperties* matPropOverride = nullptr);
//
//		//Handle addMaterial(const glm::vec4& surfaceColor, const glm::vec4& hightlightColor, const glm::vec4& ambient,
//		//	const glm::vec4& diffuse, const glm::vec4& specular,
//		//	const int& shinyCoefficient, Handle* texture, Handle* bumpMap);
//		/// <summary>
//		/// Create a light object with a linked game object
//		/// </summary>
//		/// <param name="type"></param>
//		/// <param name="position"></param>
//		/// <param name="linkedObject"></param>
//		/// <param name="ambient"></param>
//		/// <param name="diffuse"></param>
//		/// <param name="specular"></param>
//		/// <param name="direction"></param>
//		/// <param name="innerCutoff"></param>
//		/// <param name="outerCutoff"></param>
//		/// <returns></returns>
//		Handle addLight(const Type::Light& type, const glm::vec3& position, const Handle& linkedHandle,
//			const glm::vec4& ambient, const glm::vec4& diffuse,
//			const glm::vec4& specular, const glm::vec4* direction = nullptr,
//			const float* innerCutoff = nullptr, const float* outerCutoff = nullptr);
//		/// <summary>
//		/// Create a light object with no linked game object
//		/// </summary>
//		/// <param name="type"></param>
//		/// <param name="position"></param>
//		/// <param name="ambient"></param>
//		/// <param name="diffuse"></param>
//		/// <param name="specular"></param>
//		/// <param name="direction"></param>
//		/// <param name="innerCutoff"></param>
//		/// <param name="outerCutoff"></param>
//		/// <returns></returns>
//		Handle addLight(const Type::Light& type, const glm::vec3& position, const glm::vec4& ambient,
//			const glm::vec4& diffuse, const glm::vec4& specular,
//			const glm::vec4* direction = nullptr, const float* innerCutoff = nullptr,
//			const float* outerCutoff = nullptr);
//
//		friend class GameObjects::Builder;
//	};
//}