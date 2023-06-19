#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "Defs.h"

#include<iostream>
#include <optional>
#include <limits>


/* STRUCTS */
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapchainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

	
/* FEATURES */
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> requiredExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

/* CONSTANTS */
const int DEFAULT_WIDTH = 1920;
const int DEFAULT_HEIGHT = 1080;
const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
const bool pickWorseDevice = false;

/* APPLICATION INFO */
const char applicationName[] = "Pyrrhasterized: a porte-folio rasterized renderer";
const uint32_t applicationVersion = 0;

class VulkanContext
{
private:

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	vma::Allocator m_allocator;
	vk::Instance m_instance = VK_NULL_HANDLE;
	vk::PhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	vk::Device m_device = VK_NULL_HANDLE;
	QueueFamilyIndices m_queueIndices;
	vk::Queue m_graphicsQueue = VK_NULL_HANDLE;
	vk::Queue m_presentQueue = VK_NULL_HANDLE;
	vk::CommandPool m_commandPool = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	vk::SwapchainKHR m_swapchain = VK_NULL_HANDLE;
	std::vector<vk::Image> m_swapchainImages;
	std::vector<vk::ImageView> m_swapchainImageViews;
	vk::Format m_swapchainFormat = vk::Format::eUndefined;
	vk::Extent2D m_swapchainExtent;



	vk::SurfaceFormatKHR m_surfaceFormat;
	GLFWwindow* m_window;

	bool m_framebufferResized = false;
	
	vk::DescriptorPool m_imGUIDescriptorPool = VK_NULL_HANDLE;

public:
	VulkanContext();
	~VulkanContext();

	//ACCESSORS
	[[nodiscard]] vk::Device getDevice();
	[[nodiscard]] vma::Allocator getAllocator();

	//SWAPCHAIN
	[[nodiscard]] vk::Extent2D getSwapchainExtent() const;
	[[nodiscard]] vk::Format getSwapchainFormat() const;
	[[nodiscard]] uint32_t getSwapchainImagesCount() const;
	[[nodiscard]] const std::vector<vk::ImageView> getSwapchainImageViews();
	[[nodiscard]] uint32_t acquireNextSwapchainImage(vk::Semaphore &imageAvailableSemaphore);
	[[nodiscard]] vk::SwapchainKHR getSwapchain() const;

	void recreateSwapchain();
	void cleanupSwapchain();
	bool isFramebufferResized();
	void clearFramebufferResized();
	
	//QUEUES
	[[nodiscard]] QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& physicalDevice) const;
	[[nodiscard]] QueueFamilyIndices findQueueFamilies() const;
	[[nodiscard]] vk::Queue getGraphicsQueue();
	[[nodiscard]] vk::Queue getPresentQueue();

	//COMMAND POOL
	[[nodiscard]] vk::CommandPool getCommandPool();

	//FEATURES AND PROPERTIES
	[[nodiscard]] vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlagBits features);
	[[nodiscard]] vk::SampleCountFlagBits getMaxUsableSampleCount()const;

	//WINDOW
	bool isWindowOpen() const;
	void manageWindow();
	GLFWwindow* getWindowPtr();

	//BUFFERS
	[[nodiscard("Release the allocation when the buffer is no longer used")]] std::pair<vk::Buffer, vma::Allocation> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage);
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

	//COMMAND BUFFERS
	[[nodiscard("Call endSingleTimeCommands(returnValue) to end and submit the buffer")]] vk::CommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

	//PROPERTIES
	[[nodiscard]] vk::PhysicalDeviceProperties getProperties()const;
	[[nodiscard]] vk::FormatProperties getFormatProperties(vk::Format format)const;

	//IMGUI
	[[nodiscard]]ImGui_ImplVulkan_InitInfo getImGuiInitInfo();


	void setDebugObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name);
private:
	//VALIDATION LAYERS
	[[nodiscard]] bool checkValidationLayerSupport();
	void createDebugMessenger();
	[[nodiscard]] vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();

	//EXTENSIONS
	[[nodiscard]] std::vector<const char*> getRequiredExtensions();
	[[nodiscard]] bool checkDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice, std::vector<const char*> extensions);


	//INSTANCE
	void createInstance();

	//PHYSICAL DEVICE
	void pickPhysicalDevice();
	[[nodiscard]] bool isDeviceSuitable(const vk::PhysicalDevice& device);
	[[nodiscard]] vk::PhysicalDevice getBestDevice(std::vector<vk::PhysicalDevice> devices);


	//LOGICAL DEVICE
	void createLogicalDevice();

	//COMMAND POOL
	void createCommandPool();

	//SURFACE
	void createSurface();
	void createWindow();


	//SWAPCHAIN
	void createSwapchain();
	[[nodiscard]] SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice& physicalDevice);
	[[nodiscard]] vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	[[nodiscard]] vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes);
	[[nodiscard]] vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR capabilities);
	void createSwapchainImageViews();
	

	//ALLOCATORS
	void createAllocator();
	
	//FunctionPointers
	PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag;
	PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
	PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin;
	PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd;
	PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert;


};

