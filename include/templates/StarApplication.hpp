#pragma once

#include "FileResourceManager.hpp"
#include "Shader.hpp"
#include "ConfigFile.hpp"
#include "Handle.hpp"
#include "Camera.hpp"
#include "Time.hpp"

#include <GLFW/glfw3.h>

#include <memory> 
#include <string> 
#include <vector>

namespace star {
    template<typename shaderManagerT, typename textureManagerT, typename lightManagerT, typename sceneBuilderT>
    class StarApplication {
    public:
        StarApplication(ConfigFile* configFile, std::vector<Handle>* objectList, std::vector<Handle>& lightList, shaderManagerT* shaderManager,
            textureManagerT* textureManager, lightManagerT* lightManager, sceneBuilderT& sceneManager,
            Camera* inCamera) :
            lightList(lightList), 
            configFile(configFile),
            objectList(objectList),
            shaderManager(shaderManager),
            sceneBuilder(sceneManager),
            textureManager(textureManager),
            lightManager(lightManager),
            camera(inCamera) { }

        virtual ~StarApplication() {};

        virtual void Load() = 0;

        virtual void Update() = 0;

        std::vector<Handle> getLights() { return lightList; }

        lightManagerT* getLightManager() { return lightManager; }

        ConfigFile* configFile;

    protected:
        sceneBuilderT& sceneBuilder;
        shaderManagerT* shaderManager;
        textureManagerT* textureManager;
        lightManagerT* lightManager;
        std::vector<Handle>* objectList = nullptr;
        std::vector<Handle>& lightList = nullptr;
        Camera* camera;

        Time time = Time();
    private:

    };
}
