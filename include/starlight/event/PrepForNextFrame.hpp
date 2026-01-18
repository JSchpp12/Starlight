#pragma once

#include <star_common/FrameTracker.hpp>
#include <star_common/IEvent.hpp>

#include <string_view>

namespace star::event
{
constexpr std::string_view GetPrepForNextFrameEventTypeName = "star::event::FramePrep";

class PrepForNextFrame : public common::IEvent
{
  public:
    explicit PrepForNextFrame(common::FrameTracker &frameTracker);
    virtual ~PrepForNextFrame() = default;

    common::FrameTracker *getFrameTracker() const
    {
        return m_frameTracker;
    }

  private:
    mutable common::FrameTracker *m_frameTracker = nullptr;
};
} // namespace star::event