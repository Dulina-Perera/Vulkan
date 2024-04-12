#include "pipeline.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace VulkanEngine {
    Pipeline::Pipeline(const std::string &vertexShaderFile, const std::string &fragmentShaderFile) {
        createGraphicsPipeline(vertexShaderFile, fragmentShaderFile);
    }

    Pipeline::~Pipeline() {
    }

    std::vector<char> Pipeline::readFile(const std::string &file) {
        std::ifstream in(file, std::ios::ate | std::ios::binary);
        if (!in.is_open()) {
            throw std::runtime_error("Failed to open file: " + file);
        }

        size_t fileSize = static_cast<size_t>(in.tellg());
        std::vector<char> buffer(fileSize);

        in.seekg(0);
        in.read(buffer.data(), fileSize);

        in.close();

        return buffer;
    }

    void Pipeline::createGraphicsPipeline(const std::string &vertexShaderFile, const std::string &fragmentShaderFile) {
        auto vertexShaderCode = readFile(vertexShaderFile);
        auto fragmentShaderCode = readFile(fragmentShaderFile);

        std::cout << "Vertex Shader Code: " << vertexShaderCode.size() << std::endl;
        std::cout << "Fragment Shader Code: " << fragmentShaderCode.size() << std::endl;
    }
}
