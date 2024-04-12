#pragma once

#include "pipeline.hpp"
#include "window.hpp"

namespace VulkanEngine {
    class App {
    private:
        Window window{HEIGHT, WIDTH, "Vulkan Engine"};
        Pipeline pipeline{"shaders/build/simple_shader.vert.spv", "shaders/build/simple_shader.frag.spv"};
    public:
        static constexpr size_t WIDTH = 800, HEIGHT = 600;
        
        void run();
    };
}
