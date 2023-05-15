#include "VulkanContext.h"
#include <set>


//Proxy function around vkCreateDebugUtilsMessengerEXT
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

//Proxy function around vkDestroyDebugUtilsMessengerEXT
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}


VulkanContext::VulkanContext()
{
	createWindow();
	createInstance();
	createDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	pfnDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(m_device, "vkDebugMarkerSetObjectTagEXT");
	pfnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(m_device, "vkDebugMarkerSetObjectNameEXT");
	pfnCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerBeginEXT");
	pfnCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerEndEXT");
	pfnCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerInsertEXT");
	createCommandPool();
	createAllocator();
	createSwapchain();
}


VulkanContext::~VulkanContext()
{

	for (auto imageView : m_swapchainImageViews) {
		m_device.destroyImageView(imageView);
	}

	m_device.destroySwapchainKHR(m_swapchain);
	m_device.destroyCommandPool(m_commandPool);

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(static_cast<VkInstance>(m_instance), m_debugMessenger, nullptr);
	}
	vkDestroyDescriptorPool(m_device, m_imGUIDescriptorPool, nullptr);
	ImGui_ImplVulkan_Shutdown();

	glfwDestroyWindow(m_window);
	glfwTerminate();
	m_instance.destroySurfaceKHR(m_surface);
	m_allocator.destroy();
	m_device.destroy();
	m_instance.destroy();

}

#pragma region VALIDATION_LAYERS
//Checks if the constant list of validation layers is supported
bool VulkanContext::checkValidationLayerSupport()
{
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
		}
	}
	return true;

}
/*	Debug Callback function
	PFN_vkDebugUtilsMessengerCallbackEXT signature
	VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to use it
*/
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData
) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}




//Populates the DebugUtilsMessengerCreateInfo
vk::DebugUtilsMessengerCreateInfoEXT VulkanContext::populateDebugMessengerCreateInfo() {
	return vk::DebugUtilsMessengerCreateInfoEXT{
		.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT,
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
		.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		.pfnUserCallback = debugCallback,
	};
}

void VulkanContext::createDebugMessenger() {
	if (!enableValidationLayers) return;
	//Pick message types and severities
	vk::DebugUtilsMessengerCreateInfoEXT createInfo = populateDebugMessengerCreateInfo();
	
	// NOTE: Vulkan-hpp has methods for this, but they trigger linking errors...
	if (CreateDebugUtilsMessengerEXT(m_instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo), nullptr, &m_debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}
#pragma endregion

#pragma region EXTENSIONS
//returns a vector of char* pointing to extension names
std::vector<const char*> VulkanContext::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount); //Copies the glfwExtensions char** into a vector

	//Adds the extension required for custom callbacks for debug messages
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	return extensions;
}

//returns true if the vector of extension names is supported by the VkPhysicalDevice
bool VulkanContext::checkDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice, std::vector<const char*> extensions)
{
	std::set<std::string> checkedExtensions (extensions.begin(), extensions.end());

	for (const auto& extension : physicalDevice.enumerateDeviceExtensionProperties()) {
		checkedExtensions.erase(extension.extensionName);
	}

	return checkedExtensions.empty();

}

#pragma endregion

#pragma region INSTANCE
//Instance creation
void VulkanContext::createInstance()
{
	vk::ApplicationInfo appInfo{
	.sType = vk::StructureType::eApplicationInfo,
	.pApplicationName = applicationName,
	.applicationVersion = applicationVersion,
	.apiVersion = VK_API_VERSION_1_3,
	};


	auto extensions = getRequiredExtensions();

	vk::InstanceCreateInfo createInfo{
	.sType = vk::StructureType::eInstanceCreateInfo,
	.pApplicationInfo = &appInfo,
	.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
	.ppEnabledExtensionNames = extensions.data()
	};
	
	//Debug validation layers
	vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		debugMessengerCreateInfo = populateDebugMessengerCreateInfo();
		createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;

		//Best practices layer, added to the debugMessenger's pNext.
		vk::ValidationFeatureEnableEXT enables[] = { vk::ValidationFeatureEnableEXT::eBestPractices };
		vk::ValidationFeaturesEXT features{
			.sType = vk::StructureType::eValidationFeaturesEXT,
			.enabledValidationFeatureCount = 1,
			.pEnabledValidationFeatures = enables,
		};

		debugMessengerCreateInfo.pNext = &features;

	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vk::createInstance(&createInfo, nullptr, &m_instance) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create instance");
	}

	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not supported!");
	}

}
#pragma endregion

#pragma region PHYSICAL_DEVICE
//returns true if the physical device fits the needed properties and features. Update it for your needs
bool VulkanContext::isDeviceSuitable(const vk::PhysicalDevice &device)
{
	vk::PhysicalDeviceProperties physicalDeviceProperties = device.getProperties();
	vk::PhysicalDeviceFeatures physicalDeviceFeatures = device.getFeatures(); 

	bool extensionsSupported = checkDeviceExtensionsSupport(device, requiredExtensions);
	bool swapchainAdequate = false;
	QueueFamilyIndices indices = findQueueFamilies(device);

	if (extensionsSupported)
	{
		SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device);
		swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
	}

	//TODO Better physical device features management ( physicalDeviceFeatures.samplerAnisotropy; alone)
	return indices.isComplete() && extensionsSupported && swapchainAdequate && physicalDeviceFeatures.samplerAnisotropy && physicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing && physicalDeviceFeatures.fillModeNonSolid;

}
//finds a reference to an appropriate physical device and keeps it as a member.
void VulkanContext::pickPhysicalDevice() {
	auto physicalDevices = m_instance.enumeratePhysicalDevices();
	
	if (physicalDevices.size() == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	for (const auto& device : physicalDevices)
	{
		if (isDeviceSuitable(device))
		{
			m_physicalDevice = device;
			std::cout << "Device name: " << m_physicalDevice.getProperties().deviceName << std::endl;
			break;
		}
		if (!m_physicalDevice) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}
}
#pragma endregion

#pragma region QUEUES
//Finds queue families for the physical device passed as arguments
QueueFamilyIndices VulkanContext::findQueueFamilies(const vk::PhysicalDevice& physicalDevice) const{
	QueueFamilyIndices indices;
	auto queueFamilies = physicalDevice.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indices.graphicsFamily = i;
		}

		if (queueFamily.queueCount > 0 && physicalDevice.getSurfaceSupportKHR(i, m_surface)) {
			indices.presentFamily = i;
		}

		if (indices.isComplete())break;

		i++;
		
	}
	return indices;
}

//Finds queue families for the currently used physical device
QueueFamilyIndices VulkanContext::findQueueFamilies() const {
	QueueFamilyIndices indices;
	auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indices.graphicsFamily = i;
		}

		if (queueFamily.queueCount > 0 && m_physicalDevice.getSurfaceSupportKHR(i, m_surface)) {
			indices.presentFamily = i;
		}

		if (indices.isComplete())break;

		i++;

	}
	return indices;
}

vk::Queue VulkanContext::getGraphicsQueue() {
	return m_graphicsQueue;
}

vk::Queue VulkanContext::getPresentQueue() {
	return m_presentQueue;
}

#pragma endregion

#pragma region SURFACE
void VulkanContext::createSurface() {

	VkSurfaceKHR rawSurface;
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &rawSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
	m_surface = rawSurface;

}


//TODO Documentation
void VulkanContext::createWindow() {
	glfwInit(); // Initializes the GLFW library
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Tells GLFW that we don't need an OpenGL Context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	auto monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);


#ifdef NDEBUG
	//Fullscreen by default
	m_window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, applicationName, monitor, nullptr);
	glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#else
	//Not Fullscreen to help with debugging
	m_window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, applicationName, nullptr, nullptr); //nullptr: no monitor attached, 2nd nullptr: OpenGL related
#endif
	glfwSetWindowUserPointer(m_window, m_instance);
}


bool VulkanContext::isWindowOpen() const
{
	return !glfwWindowShouldClose(m_window);
}

void VulkanContext::manageWindow() {
	glfwPollEvents();
}
#pragma endregion

#pragma region SWAPCHAIN
const std::vector<vk::ImageView> VulkanContext::getSwapchainImageViews() {
	if (m_swapchainImageViews.size() == 0)
	{
		throw std::runtime_error("tried to retrieve swapchain image views before initializing the swapchain image views");
	}
	return m_swapchainImageViews;
}

uint32_t VulkanContext::getSwapchainImagesCount() const
{
	size_t count = m_swapchainImages.size();
	if (count == 0) {
		throw std::runtime_error("tried to retrieve swapchain image count before initializing the swapchain images");
	}
	return static_cast<uint32_t>(count);
}
vk::Format VulkanContext::getSwapchainFormat() const
{
	return m_swapchainFormat;
}

vk::Extent2D VulkanContext::getSwapchainExtent() const
{
	if (m_swapchainExtent.height != 0 && m_swapchainExtent.width != 0)
	{
		return m_swapchainExtent;
	}
	else {
		throw std::runtime_error("tried to get an undefined swapchain extent");
	}
}

bool VulkanContext::isFramebufferResized() {
	return m_framebufferResized;
}

void VulkanContext::clearFramebufferResized() {
	m_framebufferResized = false;
}

void VulkanContext::cleanupSwapchain() {
	for (auto imageView : m_swapchainImageViews)
	{
		m_device.destroyImageView(imageView);
	}
	m_device.destroySwapchainKHR(m_swapchain);
}

void VulkanContext::recreateSwapchain() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}
	createSwapchain();
}


SwapchainSupportDetails VulkanContext::querySwapchainSupport(const vk::PhysicalDevice &physicalDevice) 
{
	SwapchainSupportDetails details;
	details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
	details.formats = physicalDevice.getSurfaceFormatsKHR(m_surface);
	details.presentModes = physicalDevice.getSurfacePresentModesKHR(m_surface);

	return details;
}

vk::SurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) 
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

vk::PresentModeKHR VulkanContext::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes) 
{
	vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
		else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

vk::Extent2D VulkanContext::chooseSwapExtent(vk::SurfaceCapabilitiesKHR capabilities)
{

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		//High DPI Displays don't have pixels that match screen coordinates
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);

		vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void VulkanContext::createSwapchainImageViews()
{
	m_swapchainImageViews.resize(m_swapchainImages.size());

	vk::ImageViewCreateInfo createInfo = {
		.sType = vk::StructureType::eImageViewCreateInfo,
		.viewType = vk::ImageViewType::e2D,
		.format = m_swapchainFormat,
		.components = {
				.r = vk::ComponentSwizzle::eIdentity,
				.g = vk::ComponentSwizzle::eIdentity,
				.b = vk::ComponentSwizzle::eIdentity,
				.a = vk::ComponentSwizzle::eIdentity,
			},
		.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
	};
	for (size_t i = 0; i < m_swapchainImages.size(); i++) {	
		try 
		{
			createInfo.image = m_swapchainImages[i];
			m_swapchainImageViews[i] = m_device.createImageView(createInfo);
		}
		catch (vk::SystemError err) 
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void VulkanContext::createSwapchain() 
{
	SwapchainSupportDetails swapChainSupport = querySwapchainSupport(m_physicalDevice);

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo{
		.sType = vk::StructureType::eSwapchainCreateInfoKHR,
		.surface = m_surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.preTransform = swapChainSupport.capabilities.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = vk::SwapchainKHR(nullptr),
	};

	m_queueIndices = findQueueFamilies(m_physicalDevice);
	uint32_t queueFamilyIndices[] = { m_queueIndices.graphicsFamily.value(), m_queueIndices.presentFamily.value() };

	if (m_queueIndices.graphicsFamily != m_queueIndices.presentFamily)
	{
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else 
	{
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}

	try {
		m_swapchain = m_device.createSwapchainKHR(createInfo);
	}
	catch (vk::SystemError err) {
		throw std::runtime_error("failed to create swap chain!");
	}

	m_swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain);

	m_swapchainFormat = surfaceFormat.format;
	m_swapchainExtent = extent;

	createSwapchainImageViews();
}


uint32_t VulkanContext::acquireNextSwapchainImage(vk::Semaphore &imageAvailableSemaphore) {
	uint32_t imageIndex = 0;
	try {
		vk::ResultValue result = m_device.acquireNextImageKHR(m_swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, nullptr);
		imageIndex = result.value;
	}
	/*catch (vk::OutOfDateKHRError err) {
		renderer->recreateSwapchainSizedObjects();
		return imageIndex;
	}*/
	catch (vk::SystemError err) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	return imageIndex;
}
vk::SwapchainKHR VulkanContext::getSwapchain() const {
	return m_swapchain;
}
#pragma endregion

#pragma region DEVICE
vk::Device VulkanContext::getDevice()
{
	return m_device;
}

void VulkanContext::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {

		queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{
		.sType = vk::StructureType::eDeviceQueueCreateInfo,
		.queueFamilyIndex = queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
			});
	}



	//TODO Better Device Features management
	vk::PhysicalDeviceFeatures deviceFeatures{
		.sampleRateShading = VK_TRUE,
		.fillModeNonSolid = VK_TRUE,
		.samplerAnisotropy = VK_TRUE,
	};

	vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{
		.sType = vk::StructureType::ePhysicalDeviceDescriptorIndexingFeatures,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
	}
;

	vk::DeviceCreateInfo createInfo{
	.sType = vk::StructureType::eDeviceCreateInfo,
	.pNext = &descriptorIndexingFeatures,
	.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
	.pQueueCreateInfos = queueCreateInfos.data(),
	.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
	.ppEnabledExtensionNames = requiredExtensions.data(),
	.pEnabledFeatures = &deviceFeatures,

	};
	
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (m_physicalDevice.createDevice(&createInfo, nullptr, &m_device) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create logical device !");
	}

	m_device.getQueue(indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	m_device.getQueue(indices.presentFamily.value(), 0, &m_presentQueue);
}

#pragma endregion

#pragma region FEATURES_AND_PROPERTIES
vk::Format VulkanContext::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlagBits features)
{
	for (vk::Format format : candidates)
	{
		vk::FormatProperties props = m_physicalDevice.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
		throw std::runtime_error("failed to find supported format!");
	}

}
vk::SampleCountFlagBits VulkanContext::getMaxUsableSampleCount() const
{
	vk::PhysicalDeviceProperties properties = m_physicalDevice.getProperties();

	vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

	if (counts & vk::SampleCountFlagBits::e64)
	{
		return vk::SampleCountFlagBits::e64;
	}
	if (counts & vk::SampleCountFlagBits::e32)
	{
		return vk::SampleCountFlagBits::e32;
	}
	if (counts & vk::SampleCountFlagBits::e16)
	{
		return vk::SampleCountFlagBits::e16;
	}
	if (counts & vk::SampleCountFlagBits::e8)
	{
		return vk::SampleCountFlagBits::e8;
	}
	if (counts & vk::SampleCountFlagBits::e4)
	{
		return vk::SampleCountFlagBits::e4;
	}
	if (counts & vk::SampleCountFlagBits::e2)
	{
		return vk::SampleCountFlagBits::e2;
	}
	return vk::SampleCountFlagBits::e1;
}

#pragma endregion 

#pragma region ALLOCATORS
void VulkanContext::createAllocator() {
	vma::AllocatorCreateInfo createInfo{
		.physicalDevice = m_physicalDevice,
		.device = m_device,
		.instance = m_instance,
	};

	m_allocator = vma::createAllocator(createInfo);
}

vma::Allocator VulkanContext::getAllocator()
{
	return m_allocator;
}

#pragma endregion

#pragma region BUFFERS
std::pair<vk::Buffer, vma::Allocation> VulkanContext::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage) 
{
	vk::BufferCreateInfo bufferInfo{
		.sType = vk::StructureType::eBufferCreateInfo,
		.size = size,
		.usage = usage, 
		.sharingMode = vk::SharingMode::eExclusive,
	};
	
	vma::AllocationCreateInfo bufferAllocInfo{
		.usage = memoryUsage,
	};

	if (usage == vk::BufferUsageFlagBits::eTransferSrc)
	{
		bufferAllocInfo.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
	}

	return m_allocator.createBuffer(bufferInfo, bufferAllocInfo);
}

void VulkanContext::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::BufferCopy copyRegion{
		.size = size,
	};
	commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);
	endSingleTimeCommands(commandBuffer);
}

void VulkanContext::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {

	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::BufferImageCopy region{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0, //0 : Simply tightly packed
		.imageSubresource = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = {0, 0, 0},
		.imageExtent = {
			.width = width,
			.height = height,
			.depth = 1,
		}
	};

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region); 

	endSingleTimeCommands(commandBuffer);

}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	//finds the first memory type id in the typeFilter that has all the property flags
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}
#pragma endregion

#pragma region COMMAND_BUFFERS
vk::CommandBuffer VulkanContext::beginSingleTimeCommands() {

	vk::CommandBufferAllocateInfo allocInfo{
		.sType = vk::StructureType::eCommandBufferAllocateInfo,
		.commandPool = m_commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1,
	};

	vk::CommandBuffer commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo{
		.sType = vk::StructureType::eCommandBufferBeginInfo,
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
	};

	commandBuffer.begin(beginInfo);
	return commandBuffer;

}
void VulkanContext::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
	commandBuffer.end();

	vk::SubmitInfo submitInfo{
		.sType = vk::StructureType::eSubmitInfo,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};

	m_graphicsQueue.submit(submitInfo);
	m_graphicsQueue.waitIdle();
	m_device.freeCommandBuffers(m_commandPool, commandBuffer);
}
#pragma endregion

#pragma region properties
GLFWwindow* VulkanContext::getWindowPtr()
{
	return m_window;
}
vk::PhysicalDeviceProperties VulkanContext::getProperties() const
{
	return m_physicalDevice.getProperties();
}

vk::FormatProperties VulkanContext::getFormatProperties(vk::Format format) const
{
	return m_physicalDevice.getFormatProperties(format);
}
#pragma endregion

#pragma region COMMAND_POOL
void VulkanContext::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies();

	vk::CommandPoolCreateInfo poolInfo{
		.sType = vk::StructureType::eCommandPoolCreateInfo,
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, //Hint that command buffers are rerecorded with new commands very often (VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : Allow command buffers to be rerecorded individually, without this flag they all have to be reset together)
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
	};

	try {
		m_commandPool = m_device.createCommandPool(poolInfo);
	}
	catch (vk::SystemError err)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

vk::CommandPool VulkanContext::getCommandPool() {
	return m_commandPool;
}
#pragma endregion

#pragma region IMGUI
ImGui_ImplVulkan_InitInfo VulkanContext::getImGuiInitInfo() {
	//TODO better sized imgui pool ?
	vk::DescriptorPoolSize poolSizes[] =
	{
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 }
	};


	vk::DescriptorPoolCreateInfo poolInfo = {
		.sType = vk::StructureType::eDescriptorPoolCreateInfo,
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = 1000,
		.poolSizeCount = std::size(poolSizes),
		.pPoolSizes = poolSizes,
	};

	try {
		m_imGUIDescriptorPool = m_device.createDescriptorPool(poolInfo);
	}
	catch (vk::SystemError err)
	{
		throw std::runtime_error("could not create descriptor pool");
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForVulkan(m_window, true);
	
	ImGui_ImplVulkan_InitInfo initGuiInfo{
		.Instance = m_instance,
		.PhysicalDevice = m_physicalDevice,
		.Device = m_device,
		.QueueFamily = m_queueIndices.graphicsFamily.value(),
		.Queue = m_graphicsQueue,
		.DescriptorPool = m_imGUIDescriptorPool,
		.Subpass = 0,
		.MinImageCount = MAX_FRAMES_IN_FLIGHT,
		.ImageCount = MAX_FRAMES_IN_FLIGHT,
		.MSAASamples = static_cast<VkSampleCountFlagBits>(getMaxUsableSampleCount()),

	};

	return initGuiInfo;
}
#pragma engregion IMGUI

void VulkanContext::setDebugObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name)
{

	// Check for a valid function pointer
	if (pfnDebugMarkerSetObjectName)
	{
		VkDebugMarkerObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.object = object;
		nameInfo.pObjectName = name;
		pfnDebugMarkerSetObjectName(m_device, &nameInfo);
	}
}