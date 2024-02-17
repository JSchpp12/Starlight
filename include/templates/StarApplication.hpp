#pragma once

#include "BasicCamera.hpp"
#include "Handle.hpp"
#include "Time.hpp"
#include "Interactivity.hpp"
#include "StarObject.hpp"
#include "StarScene.hpp"
#include "StarShader.hpp"
#include "SwapChainRenderer.hpp"

#include <GLFW/glfw3.h>

#include <memory> 
#include <string> 
#include <vector>


namespace star {
    class StarApplication : public Interactivity {
    public:
        StarApplication(StarScene& scene)
            : scene(scene), camera(1280, 720){ 
            this->registerInteractions();
        }

        virtual std::string getApplicationName() = 0; 

        virtual ~StarApplication() {};

        virtual void onWorldUpdate() = 0;

        virtual void onKeyPress(int key, int scancode, int mods) override {};

        virtual void onKeyRelease(int key, int scancode, int mods) override {};

        virtual void onMouseMovement(double xpos, double ypos) override {};

        virtual void onMouseButtonAction(int button, int action, int mods) override {};

        virtual void onScroll(double xoffset, double yoffset) override {};

        virtual std::unique_ptr<SwapChainRenderer> getMainRenderer(StarDevice& device, StarWindow& window);

        virtual StarCamera& getCamera() { return camera; }

        virtual std::vector<star::Rendering_Features> getRequiredDeviceExtensions() { return std::vector<star::Rendering_Features>(); };

    protected:
        BasicCamera camera;
        StarScene& scene; 
        Time time = Time();
    };
}
