#pragma once

#include <optional>

namespace star{
    class ManagerController{
        public:
        ManagerController() = default;
        ManagerController(const uint8_t& frameInFlightIndexToUpdateOn) : frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn){}

        virtual bool isValid(const uint8_t& currentFrameInFlightIndex) const; 

        const std::optional<uint8_t>& getFrameInFlightIndexToUpdateOn() const { return frameInFlightIndexToUpdateOn; }
        private:
        std::optional<uint8_t> frameInFlightIndexToUpdateOn = std::nullopt;
    };
}