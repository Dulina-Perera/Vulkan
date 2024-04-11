#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace VulkanEngine {
    class Window {
    private:
        GLFWwindow *window;
        const size_t height;
        const size_t width;
        std::string title;

        void initWindow();
    public:
        Window(const size_t height, const size_t width, const std::string title);
        ~Window();

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        bool shouldClose() const { return glfwWindowShouldClose(window); }
    };
}
