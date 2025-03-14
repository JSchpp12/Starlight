#pragma once

#include <optional>
#include <memory>

namespace star::ManagerController{
    template <typename T>
    class Controller{
        public:
        Controller() = default;
        Controller(const uint8_t& frameInFlightIndexToUpdateOn) : frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn){}

        virtual bool isValid(const uint8_t& currentFrameInFlightIndex) const{
            if (this->frameInFlightIndexToUpdateOn.has_value() && currentFrameInFlightIndex == this->frameInFlightIndexToUpdateOn.value())
                return false;
            return true;
        };

        virtual std::unique_ptr<T> createTransferRequest() = 0;

        const std::optional<uint8_t>& getFrameInFlightIndexToUpdateOn() const { return frameInFlightIndexToUpdateOn; }
        private:
        std::optional<uint8_t> frameInFlightIndexToUpdateOn = std::nullopt;
    };
}