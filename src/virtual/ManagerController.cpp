#include "ManagerController.hpp"

bool star::ManagerController::isValid(const uint8_t& currentFrameInFlightIndex) const{
    if (this->frameInFlightIndexToUpdateOn.has_value() && currentFrameInFlightIndex == this->frameInFlightIndexToUpdateOn.value())
        return false;

    return true;
}