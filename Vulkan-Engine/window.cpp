#include "window.hpp"

namespace VulkanEngine {
    Window::Window(
        const size_t height, 
        const size_t width, 
        const std::string title
    ) : height(height), width(width), title(title) {
        initWindow();
    }

    Window::~Window() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Window::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    }
}
