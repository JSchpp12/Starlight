#pragma once

#include "StarApplication.hpp"
#include "StarScene.hpp"
#include "StarWindow.hpp"
#include "TransferWorker.hpp"
#include "core/SystemContext.hpp"

#include <memory>
#include <string>
#include <vector>

namespace star
{
class StarEngine
{
  public:
    StarEngine(StarApplication &application);

    ~StarEngine();

    void run();

  protected:
    StarApplication &m_application;
    Handle defaultDevice = Handle{.type = star::Handle_Type::device, .id = 0};
    uint64_t frameCounter = 0;
    std::unique_ptr<StarWindow> window = nullptr;
    core::SystemContext deviceManager;
    std::shared_ptr<StarScene> currentScene = nullptr;

  private:
    const bool OVERRIDE_APPLY_SINGLE_THREAD_MODE = false;

    static std::unique_ptr<StarWindow> CreateStarWindow();

    static std::unique_ptr<job::TaskManager> CreateManager();

    static uint8_t GetNumFramesInFlight();

    void registerScreenshotService(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);
};
} // namespace star