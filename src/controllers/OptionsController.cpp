#include "OptionsController.hpp"

namespace star {

OptionsController::OptionsController(RenderOptions& renderOptions) : renderOptions(renderOptions)
{
	this->printHelp();
	this->registerInteractions(); 
}
void OptionsController::onKeyPress(int key, int scancode, int mods)
{
	if (key == GLFW_KEY_0) {
		std::cout << "Enabling standard draw" << std::endl;
		renderOptions.setRenderMaterialSetting(Render_Settings::Material::none);
	}
	else if (key == GLFW_KEY_1) {
		std::cout << "Enabling render material option: draw ambient property" << std::endl;
		renderOptions.setRenderMaterialSetting(Render_Settings::Material::ambient);
	}
	else if (key == GLFW_KEY_2) {
		std::cout << "Enabling render material option: draw diffuse property" << std::endl;
		renderOptions.setRenderMaterialSetting(Render_Settings::Material::diffuse);
	}
	else if (key == GLFW_KEY_3) {
		std::cout << "Enabling render material option: draw specular property" << std::endl;
		renderOptions.setRenderMaterialSetting(Render_Settings::Material::specular);
	}
	else if (key == GLFW_KEY_4) {
		std::cout << "Enabling render material option: draw bump map" << std::endl;
		renderOptions.setRenderMaterialSetting(Render_Settings::Material::bumpMap);
	}
	else if (key == GLFW_KEY_5) {
		if (renderOptions.isFeatureEnabled(Render_Settings::Feature::bumpMapping)) {
			std::cout << "Disabling Bump Mapping Feature" << std::endl;
			renderOptions.setRenderFeature(Render_Settings::Feature::bumpMapping, false);
		}
		else {
			std::cout << "Enabling Bump Mapping Feature" << std::endl;
			renderOptions.setRenderFeature(Render_Settings::Feature::bumpMapping, true);
		}
	}
}

void OptionsController::printHelp()
{
	std::cout << "Default options controller has been enabled..." << std::endl;
	std::cout << "Controls: " << std::endl;
	std::cout << "NUM: 0 - Normal Draw" << std::endl;
	std::cout << "NUM: 1 - Draw Ambient Properties" << std::endl;
	std::cout << "NUM: 2 - Draw Diffuse Properties" << std::endl;
	std::cout << "NUM: 3 - Draw Specular Properties" << std::endl;
	std::cout << "NUM: 4 - Draw Bump Map" << std::endl;
	std::cout << "NUM: 5 - Normal Draw Without Bump Mapping" << std::endl;
}

void OptionsController::onKeyRelease(int key, int scancode, int mods)
{
}

void OptionsController::onMouseMovement(double xpos, double ypos)
{
}

void OptionsController::onMouseButtonAction(int button, int action, int mods)
{
}

void OptionsController::onScroll(double xoffset, double yoffset)
{
}

void OptionsController::onWorldUpdate()
{
}

}