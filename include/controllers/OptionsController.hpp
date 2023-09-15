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
		virtual void keyCallback(int key, int scancode, int action, int mods) override;

	private:
		RenderOptions& renderOptions;
		static void printHelp();
	};
}