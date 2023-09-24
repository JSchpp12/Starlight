#pragma once 

#include "InteractionSystem.hpp"

#include <GLFW/glfw3.h>

#include <memory>
#include <optional>
#include <functional>
#include <iostream>

namespace star {
//base class to inherit from if an application wishes to handle user interaction such as keyboard callbacks and mouse movement
class Interactivity {
public:

	virtual void onKeyPress(int key, int scancode, int mods) = 0;

	virtual void onKeyRelease(int key, int scancode, int mods) = 0; 

	/// <summary>
	/// Function that will be called when mouse movement is registered by the windowing system
	/// </summary>
	/// <param name="xpos">Current x position of the mouse</param>
	/// <param name="ypos">Current y position of the mouse</param>
	virtual void onMouseMovement(double xpos, double ypos) = 0;

	/// <summary>
	/// Function that will be called when a mouse button interaction is registered by the windowing system
	/// </summary>
	/// <param name="button">GLFW key code for the button pressed</param>
	/// <param name="action">GLFW action</param>
	/// <param name="mods"></param>
	virtual void onMouseButtonAction(int button, int action, int mods) = 0;

	/// <summary>
	/// Function that will be called when a mouse wheel scroll interaction is registered by the windowing system. 
	/// </summary>
	/// <param name="xoffset"></param>
	/// <param name="yoffset"></param>
	virtual void onScroll(double xoffset, double yoffset) = 0;

	/// <summary>
	/// Function that will be called before the user defined application world update function
	/// </summary>
	virtual void onWorldUpdate() = 0;

	/// <summary>
	/// Register all ineractions in this class with the interaction system. This function MUST be called from every inheriting class
	/// </summary>
	virtual void registerInteractions();

protected:

private:
	/// <summary>
	/// Function that will be called when a keyboard interaction is registered by the windowing system
	/// </summary>
	/// <param name="key">GLFW keycode</param>
	/// <param name="scancode"></param>
	/// <param name="action">GLFW action</param>
	/// <param name="mods"></param>
	virtual void onKeyAction(int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS) {
			onKeyPress(key, scancode, mods); 
		}
		else if (action == GLFW_RELEASE) {
			onKeyRelease(key, scancode, mods); 
		}
	};
};
}