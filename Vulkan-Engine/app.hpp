#pragma once

#include "window.hpp"

namespace VulkanEngine {
    class App {
    private:
        Window window{HEIGHT, WIDTH, "Vulkan Engine"};
    public:
        static constexpr size_t WIDTH = 800, HEIGHT = 600;
        
        void run();
    };
}
