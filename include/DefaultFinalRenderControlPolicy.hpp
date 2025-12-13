#pragma once

#include "StarApplication.hpp"
#include "StarWindow.hpp"

#include <starlight/common/Handle.hpp>
 
namespace star
{
class DefaultFinalRenderControlPolicy
{
  public:
    DefaultFinalRenderControlPolicy() : m_window(CreateStarWindow())
    {
    }

    void init(StarApplication &application);

    std::unique_ptr<star::StarWindow> m_window;

  private:
    static std::unique_ptr<star::StarWindow> CreateStarWindow();
};
} // namespace star