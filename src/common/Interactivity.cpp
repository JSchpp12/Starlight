#include "Interactivity.hpp"

namespace star {
void Interactivity::registerInteractions()
{
	auto keyCallback = std::make_unique<std::function<void(int, int, int, int)>>(std::bind(&Interactivity::onKeyAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	InteractionSystem::registerKeyCallback(std::move(keyCallback));

	auto mouseMovementCallback = std::make_unique<std::function<void(double, double)>>(std::bind(&Interactivity::onMouseMovement, this, std::placeholders::_1, std::placeholders::_2));
	InteractionSystem::registerMouseMovementCallback(std::move(mouseMovementCallback));

	auto mouseButtonCallback = std::make_unique<std::function<void(int, int, int)>>(std::bind(&Interactivity::onMouseButtonAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	InteractionSystem::registerMouseButtonCallback(std::move(mouseButtonCallback));

	auto scrollCallback = std::make_unique<std::function<void(double, double)>>(std::bind(&Interactivity::onScroll, this, std::placeholders::_1, std::placeholders::_2));
	InteractionSystem::registerMouseScrollCallback(std::move(scrollCallback));

	auto worldUpdateCallback = std::make_unique<std::function<void(const uint32_t&)>>(std::bind(&Interactivity::onWorldUpdate, this, std::placeholders::_1));
	InteractionSystem::registerWorldUpdateCallback(std::move(worldUpdateCallback));
}
}