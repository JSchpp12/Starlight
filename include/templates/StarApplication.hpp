#pragma once

#include "CameraController.hpp"
#include "StarShader.hpp"
#include "Handle.hpp"
#include "Camera.hpp"
#include "Time.hpp"
#include "Interactivity.hpp"

#include <GLFW/glfw3.h>

#include <memory> 
#include <string> 
#include <vector>

namespace star {
    class StarApplication : public Interactivity {
    public:
        StarApplication(){ }

        virtual ~StarApplication() {};

        virtual void Load() = 0;

        virtual void Update() = 0;

        virtual void onKeyPress(int key, int scancode, int mods) override {};

        virtual void onKeyRelease(int key, int scancode, int mods) override {};

        virtual void onMouseMovement(double xpos, double ypos) override {};

        virtual void onMouseButtonAction(int button, int action, int mods) override {};

        virtual void onScroll(double xoffset, double yoffset) override {};

        std::vector<Handle> getLights() { return lightList; }

        std::vector<Handle> getObjects() { return objectList; }

        virtual Camera& getCamera() { return camera;  }

    protected:
        CameraController camera;

        std::vector<Handle> lightList;
        std::vector<Handle> objectList; 

        Time time = Time();

    private:

    };
}
