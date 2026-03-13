#pragma once

#include <star_common/IEvent.hpp>

#include <string_view>

namespace star::event
{
namespace frame_complete
{
constexpr const char *GetUniqueTypeName()
{
    return "EvtFC";
}
} // namespace frame_complete

class FrameComplete : public common::IEvent
{
  public:
    static constexpr std::string_view GetUniqueTypeName()
    {
        return frame_complete::GetUniqueTypeName();
    }
    FrameComplete();

  private:
};
} // namespace star::event