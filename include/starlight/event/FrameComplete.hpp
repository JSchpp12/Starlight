#pragma once

#include <star_common/IEvent.hpp>

#include <string_view>

namespace star::event
{
inline constexpr std::string_view GetFrameCompleteTypeName = "star::event::FrameComplete";

class FrameComplete : public common::IEvent
{
  public:
    FrameComplete();

    private:
};
}