#pragma once

#include "BasicCamera.hpp"
#include "Handle.hpp"
#include "Time.hpp"
#include "Interactivity.hpp"
#include "StarObject.hpp"
#include "StarScene.hpp"
#include "StarShader.hpp"
#include "BasicRenderer.hpp"

#include <GLFW/glfw3.h>

#include <memory> 
#include <string> 
#include <vector>


namespace star {
    class StarApplication : public Interactivity {
    public:
        StarApplication(StarScene& scene)
            : scene(scene){ 
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

        virtual std::unique_ptr<StarRenderer> getRenderer(StarDevice& device, StarWindow& window, RenderOptions& options);

        virtual StarCamera& getCamera() { return camera;  }

    protected:
        BasicCamera camera;
        StarScene& scene; 
        Time time = Time();

    private:

    };
}
