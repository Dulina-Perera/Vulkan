#pragma once

#include <string>
#include <vector>

namespace VulkanEngine {
    class Pipeline {
    private:
        static std::vector<char> readFile(const std::string &file);
        void createGraphicsPipeline(const std::string &vertexShaderFile, const std::string &fragmentShaderFile);
    public:
        Pipeline(const std::string &vertexShaderFile, const std::string &fragmentShaderFile);
        ~Pipeline();
    };
}
