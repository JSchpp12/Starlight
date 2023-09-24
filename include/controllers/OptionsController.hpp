#pragma once 
#include "Interactivity.hpp"
#include "RenderOptions.hpp"
#include "Enums.hpp"

#include <GLFW/glfw3.h>

#include <iostream>

namespace star {
	class OptionsController : public Interactivity {
	public:
		OptionsController(RenderOptions& renderOptions);
		virtual void onKeyPress(int key, int scancode, int mods) override;

	private:
		RenderOptions& renderOptions;
		static void printHelp();

		// Inherited via Interactivity
		void onKeyRelease(int key, int scancode, int mods) override;

		// Inherited via Interactivity
		void onMouseMovement(double xpos, double ypos) override;
		void onMouseButtonAction(int button, int action, int mods) override;
		void onScroll(double xoffset, double yoffset) override;
		void onWorldUpdate() override;
	};
}