#define VK_USE_PLATFORM_XCB_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
		const VkAllocationCallbacks *pAllocator,
		VkDebugUtilsMessengerEXT *pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks *pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentationFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentationFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentationModes;
};

class Triangle
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		loop();
		cleanup();
	}

private:
	GLFWwindow *window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;

	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	bool areDeviceExtensionsSupported(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const VkExtensionProperties &extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	template <typename T>
	bool areRequiredExtensionsAvailable(const T &requiredExtensions)
	{
		std::vector<VkExtensionProperties> availableExtensions = getAvailableExtensions();

		std::set<std::string> availableExtensionNames;
		for (const VkExtensionProperties &extension : availableExtensions)
		{
			availableExtensionNames.insert(extension.extensionName);
		}

		for (const char *extension : requiredExtensions)
		{
			if (availableExtensionNames.find(extension) == availableExtensionNames.end())
			{
				std::cerr << "Required extension '" << extension << "' is not available!\n";
				return false;
			}
			else
			{
				std::cout << "Required extension '" << extension << "' is available.\n";
			}
		}
		return true;
	}

	bool areRequiredExtensionsAvailable(uint32_t glfwExtensionCount, const char **glfwExtensions)
	{
		std::vector<const char *> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		return areRequiredExtensionsAvailable(requiredExtensions);
	}

	bool areValidationLayersSupported()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char *layer : validationLayers)
		{
			bool layerFound = false;

			for (const VkLayerProperties &layerProperties : availableLayers)
			{
				if (strcmp(layer, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkPresentModeKHR chooseSwapPresentationMode(const std::vector<VkPresentModeKHR> &availablePresentationModes)
	{
		for (const VkPresentModeKHR &availablePresentationMode : availablePresentationModes)
		{
			if (availablePresentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentationMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
	{
		for (const VkSurfaceFormatKHR &availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	void cleanup()
	{
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, nullptr);
		vkDestroyFence(logicalDevice, inFlightFence, nullptr);

		vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

		for (VkFramebuffer framebuffer : swapChainFramebuffers)
		{
			vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
		}

		vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

		for (VkImageView imageView : swapChainImageViews)
		{
			vkDestroyImageView(logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);

		vkDestroyDevice(logicalDevice, nullptr);

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void createCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;

		VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocateInfo, &commandBuffer);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffer!");
		}
	}

	void createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		VkResult result = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool!");
		}
	}

	void createFrameBuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (std::size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			VkImageView attachments[] = {swapChainImageViews[i]};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		// Viewports and scissors
		VkPipelineViewportStateCreateInfo viewportStateInfo{};
		viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.scissorCount = 1;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
		rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerInfo.depthClampEnable = VK_FALSE;
		rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerInfo.lineWidth = 1.0f;
		rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizerInfo.depthBiasEnable = VK_FALSE;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
		multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingInfo.sampleShadingEnable = VK_FALSE;
		multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
		colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingInfo.logicOpEnable = VK_FALSE;
		colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingInfo.attachmentCount = 1;
		colorBlendingInfo.pAttachments = &colorBlendAttachment;
		colorBlendingInfo.blendConstants[0] = 0.0f;
		colorBlendingInfo.blendConstants[1] = 0.0f;
		colorBlendingInfo.blendConstants[2] = 0.0f;
		colorBlendingInfo.blendConstants[3] = 0.0f;

		// Dynamic states
		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateInfo.pDynamicStates = dynamicStates.data();

		// Pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		VkResult result = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasterizerInfo;
		pipelineInfo.pMultisampleState = &multisamplingInfo;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlendingInfo;
		pipelineInfo.pDynamicState = &dynamicStateInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);

		vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	}

	void createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (std::size_t i = 0; i < swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			VkResult result = vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create image views!");
			}
		}
	}

	void createInstance()
	{
		if (enableValidationLayers && !areValidationLayersSupported())
		{
			throw std::runtime_error("Requested validation layers are not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		std::vector<const char *> requiredExtensions = getRequiredExtensions();

		if (!areRequiredExtensionsAvailable(requiredExtensions))
		{
			throw std::runtime_error("Some of the Vulkan instance extensions required by GLFW are not available!");
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers)
		{
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;

			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance!");
		}
	}

	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentationFamily.value()};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device!");
		}

		vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, indices.presentationFamily.value(), 0, &presentationQueue);
	}

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkResult result = vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	VkShaderModule createShaderModule(const std::vector<char> &code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

		VkShaderModule shaderModule;
		VkResult result = vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module!");
		}

		return shaderModule;
	}

	void createSurface()
	{
		// Display *display = glfwGetX11Display();
		// xcb_connection_t *connection = XGetXCBConnection(display);
		// if (!connection)
		// {
		// 	throw std::runtime_error("Failed to get XCB connection from X11 display!");
		// }

		// VkXcbSurfaceCreateInfoKHR createInfo{};
		// createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		// createInfo.connection = connection;
		// createInfo.window = static_cast<xcb_window_t>(glfwGetX11Window(window));

		// VkResult result = vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &surface);
		VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentationMode = chooseSwapPresentationMode(swapChainSupport.presentationModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentationFamily.value()};

		if (indices.graphicsFamily != indices.presentationFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentationMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
	}

	void createSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkResult result = vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image available semaphore!");
		}

		result = vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render finished semaphore!");
		}

		result = vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFence);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create in flight fence!");
		}
	}

	void drawFrame()
	{
		vkWaitForFences(logicalDevice, 1, &inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(logicalDevice, 1, &inFlightFence);
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
			void *pUserData)
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const VkQueueFamilyProperties &queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
			if (presentationSupport)
			{
				indices.presentationFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}

			i++;
		}

		return indices;
	}

	std::vector<VkExtensionProperties> getAvailableExtensions()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> avilableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, avilableExtensions.data());

		return avilableExtensions;
	}

	std::vector<const char *> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickupPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFrameBuffers();
		createCommandPool();
		createCommandBuffer();
		createSyncObjects();
	}

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
					 deviceFeatures.geometryShader &&
					 findQueueFamilies(device).isComplete();
	}

	void loop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}
	}

	void pickupPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		std::multimap<int, VkPhysicalDevice> candidates;

		for (const VkPhysicalDevice &device : devices)
		{
			int score = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}
		if (candidates.rbegin()->first > 0)
		{
			physicalDevice = candidates.rbegin()->second;

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
			std::cout << std::format("Selected Physical Device: {} (Score: {})\n", deviceProperties.deviceName, candidates.rbegin()->first) << std::endl;
		}
		else
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentationModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, nullptr);

		if (presentationModeCount != 0)
		{
			details.presentationModes.resize(presentationModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, details.presentationModes.data());
		}

		return details;
	}

	static std::vector<char> readFile(const std::string &filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file!");
		}

		std::size_t fileSize = (std::size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	int rateDeviceSuitability(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		int score = 0;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}

		score += deviceProperties.limits.maxImageDimension2D;

		bool swapChainAdequate = false;
		if (areDeviceExtensionsSupported(device))
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentationModes.empty();
		}

		if (
				deviceFeatures.geometryShader &&
				findQueueFamilies(device).isComplete() &&
				areDeviceExtensionsSupported(device) &&
				swapChainAdequate)
		{
			return score;
		}
		else
		{
			return 0;
		}
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		result = vkEndCommandBuffer(commandBuffer);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayers)
		{
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to set up debug messenger!");
		}
	}
};

int main()
{
	Triangle app;

	try
	{
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
