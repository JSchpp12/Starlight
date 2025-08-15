#pragma once

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "InteractionSystem.hpp"

#include <memory>
#include <string>

namespace star
{
class StarWindow
{
  public:
    class Builder
    {
      public:
        Builder() = default;
        ~Builder() = default;
        Builder &setWidth(const int &nWidth);
        Builder &setHeight(const int &nHeight);
        Builder &setTitle(const std::string &nTitle);
        std::unique_ptr<StarWindow> build();

      private:
        int width = 0, height = 0;
        std::string title = std::string();
    };

    ~StarWindow();

    vk::SurfaceKHR createWindowSurface(const vk::Instance &instance);

    void resetWindowResizedFlag()
    {
        this->frambufferResized = false;
    }
    bool shouldClose() const
    {
        return glfwWindowShouldClose(this->window);
    }
    vk::Extent2D getExtent() const
    {
        return {static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height)};
    }
    bool wasWindowResized() const
    {
        return this->frambufferResized;
    }
    GLFWwindow *getGLFWwindow() const
    {
        return this->window;
    }

  protected:
    StarWindow(const int &width, const int &height, const std::string &title);

    void initWindowInfo();

    static GLFWwindow *CreateGLFWWindow(const int &width, const int &height, const std::string &title);

    static void DestroyWindow(GLFWwindow *window); 

  private:
    bool frambufferResized = false;
    int width = 0, height = 0;
    GLFWwindow *window = nullptr;

    friend class Builder;
};
} // namespace star