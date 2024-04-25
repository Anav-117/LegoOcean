#include "VkConfig.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <algorithm>
#include <set>

std::vector<const char*> VulkanClass::getRequiredExtensions() {

	uint32_t glfwExtentionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtentionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;

}

bool VulkanClass::checkValidationLayerSupport() {

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

VulkanClass::VulkanClass() {

}

VulkanClass::VulkanClass(GLFWwindow* win) {

	window = win;
	createInstance();

	createSurface();

	physicalDevice = findPhysicalDevice();
	createLogicalDevice();

	createSwapChain();
	createImageViews();

	createRenderPass();
	createDescriptorSetLayout();
	createDescriptorPools();

	createGraphicsPipeline();
	createComputePipeline();

	createCommandPool();
	createCommandBuffer();

	createDepthResources();

	createFramebuffers();

	//createPosBuffer();

	createSyncObjects();

}

VulkanClass::~VulkanClass() {

	for (size_t i = 0; i < swapChain.framebuffers.size(); i++) {
		vkDestroyFramebuffer(logicalDevice, swapChain.framebuffers[i], nullptr);
	}
	for (size_t i = 0; i < swapChain.imageViews.size(); i++) {
		vkDestroyImageView(logicalDevice, swapChain.imageViews[i], nullptr);
	}

	vkDestroyImageView(logicalDevice, depthImageView, nullptr);
	vkDestroyImage(logicalDevice, depthImage, nullptr);
	vkFreeMemory(logicalDevice, depthImageMemory, nullptr);

	vkDestroySwapchainKHR(logicalDevice, swapChain.__swapChain, nullptr);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(logicalDevice, transformBuffer[i], nullptr);
		vkFreeMemory(logicalDevice, transformBufferMemory[i], nullptr);
		vkDestroyBuffer(logicalDevice, posBuffer[i], nullptr);
		vkFreeMemory(logicalDevice, posBufferMemory[i], nullptr);
		vkDestroyBuffer(logicalDevice, computeUniformBuffer[i], nullptr);
		vkFreeMemory(logicalDevice, computeUniformBufferMemory[i], nullptr);
	}

	delete basicShader;

	vkDestroyDescriptorPool(logicalDevice, computeDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, computeDescriptorSetLayout, nullptr);

	vkDestroyDescriptorPool(logicalDevice, uniformDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, transformDescriptorSetLayout, nullptr);

	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

	vkDestroyPipeline(logicalDevice, computePipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, computePipelineLayout, nullptr);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(logicalDevice, renderFinishedSempahore[i], nullptr);
		vkDestroyFence(logicalDevice, inFlightFence[i], nullptr);

		vkDestroySemaphore(logicalDevice, computeFinishedSemaphores[i], nullptr);
		vkDestroyFence(logicalDevice, computeInFlightFences[i], nullptr);
	}

	vkFreeCommandBuffers(logicalDevice, commandPool, swapChain.MAX_FRAMES_IN_FLIGHT, commandBuffer.data());
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

	vkDestroyDevice(logicalDevice, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyInstance(instance, nullptr);

}

void VulkanClass::createInstance() {

	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("Validation Layers Requested But Not Found\n");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Lego Ocean";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pNext = nullptr;

	auto extensions = getRequiredExtensions();

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	} 

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance\n");
	}

}

bool VulkanClass::findQueueFamilies(VkPhysicalDevice device) {

	uint32_t physicalDeviceQueueFamilyCount;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &physicalDeviceQueueFamilyCount, nullptr);
	
	if (physicalDeviceQueueFamilyCount == 0) {
		throw std::runtime_error("Cannot Find Any Queue Families On Physical Device\n");
	}

	std::vector<VkQueueFamilyProperties> queueFamilies(physicalDeviceQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &physicalDeviceQueueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (auto queueFamily : queueFamilies) {
		if ((queueFamily.queueFlags | VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags | VK_QUEUE_COMPUTE_BIT)) {
			QueueFamilyIndex.graphicsFamily = i;
			return true;
		}
		
		VkBool32 presentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			QueueFamilyIndex.presentFamily = i;
		}

		i++;
	}

	return false;

}

bool VulkanClass::checkSwapChainSupport(VkPhysicalDevice device) {

	SwapChainSupport details{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t numFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numFormats, nullptr);
	details.formats.resize(numFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numFormats, details.formats.data());

	uint32_t numPresentModes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentModes, nullptr);
	details.presentModes.resize(numPresentModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentModes, details.presentModes.data());

	if (!details.formats.empty() && !details.presentModes.empty()) {
		SwapChainDetails = details;
		return true;
	}

	return false;

}

VkPhysicalDevice VulkanClass::findPhysicalDevice() {

	VkPhysicalDevice selectedDevice = NULL;

	uint32_t numSupportedDevices;

	if (vkEnumeratePhysicalDevices(instance, &numSupportedDevices, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("No Supported Physical Devices Found\n");
	}

	std::vector<VkPhysicalDevice> physicalDevices(numSupportedDevices);

	vkEnumeratePhysicalDevices(instance, &numSupportedDevices, physicalDevices.data());

	for (auto device : physicalDevices) {
		
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		if (!findQueueFamilies(device) || !requiredExtensions.empty() || properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || !checkSwapChainSupport(device)) {
			continue;
		}

		selectedDevice = device;
		std::cout << properties.vendorID << " | " << properties.deviceName << " | " << properties.deviceType << " | " << properties.driverVersion << "\n";

	}

	if (selectedDevice == NULL) {
		throw std::runtime_error("Cannot Find Suitable Physical Device\n");
	}

	return selectedDevice;

}

void VulkanClass::createLogicalDevice() {

	float queuePriority = 1.0;
	std::vector<VkDeviceQueueCreateInfo> queueInfos;

	std::set<uint32_t> UniqueQueueFamilies = {QueueFamilyIndex.graphicsFamily, QueueFamilyIndex.presentFamily};

	for (auto queue : UniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pQueuePriorities = &queuePriority;
		queueInfo.queueCount = 1;
		queueInfo.queueFamilyIndex = queue;
		queueInfo.flags = 0;
		queueInfo.pNext = nullptr;
		queueInfos.push_back(queueInfo);
	}

	VkPhysicalDeviceFeatures supportedFeatures{};
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	VkPhysicalDeviceFeatures requiredFeatures{};
	requiredFeatures.geometryShader = VK_TRUE;
	requiredFeatures.fillModeNonSolid = VK_TRUE;
	requiredFeatures.wideLines = VK_TRUE;
	requiredFeatures.largePoints = VK_TRUE;

	VkDeviceCreateInfo logicalDeviceCreateInfo{};

	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.pNext = nullptr;
	logicalDeviceCreateInfo.flags = 0;
	logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
	logicalDeviceCreateInfo.pQueueCreateInfos = queueInfos.data();
	logicalDeviceCreateInfo.pEnabledFeatures = &requiredFeatures;
	if (enableValidationLayers) {
		logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		logicalDeviceCreateInfo.enabledLayerCount = 0;
		logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;
	}
	logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	logicalDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Logical Device\n");
	}

	vkGetDeviceQueue(logicalDevice, QueueFamilyIndex.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, QueueFamilyIndex.presentFamily, 0, &presentQueue);

}

VkSurfaceFormatKHR SwapChain::findSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

	for (const auto format : availableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return availableFormats[0];

}

VkPresentModeKHR SwapChain::findSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

	for (const auto& presentMode : availablePresentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;

}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}

}

void VulkanClass::createSurface() {

	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Window Surface\n");
	}

}

void VulkanClass::createSwapChain() {

	swapChain.format = swapChain.findSwapChainFormat(SwapChainDetails.formats);
	swapChain.presentMode = swapChain.findSwapChainPresentMode(SwapChainDetails.presentModes);
	swapChain.extent = swapChain.chooseSwapExtent(SwapChainDetails.capabilities, window);

	uint32_t imageCount = SwapChainDetails.capabilities.minImageCount + 1;

	if (SwapChainDetails.capabilities.maxImageCount > 0 && imageCount > SwapChainDetails.capabilities.maxImageCount) {
		imageCount = SwapChainDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainInfo{};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.surface = surface;
	swapChainInfo.minImageCount = imageCount;
	swapChainInfo.imageFormat = swapChain.format.format;
	swapChainInfo.imageColorSpace = swapChain.format.colorSpace;
	swapChainInfo.presentMode = swapChain.presentMode;
	swapChainInfo.imageExtent = swapChain.extent;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	uint32_t queueFamilyIndices[] = { QueueFamilyIndex.graphicsFamily, QueueFamilyIndex.presentFamily };
	if (QueueFamilyIndex.graphicsFamily != QueueFamilyIndex.presentFamily) {
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapChainInfo.preTransform = SwapChainDetails.capabilities.currentTransform;
	swapChainInfo.clipped = VK_TRUE;
	swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(logicalDevice, &swapChainInfo, nullptr, &swapChain.__swapChain) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Swapchain\n");
	}

	vkGetSwapchainImagesKHR(logicalDevice, swapChain.__swapChain, &imageCount, nullptr);
	swapChain.images.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain.__swapChain, &imageCount, swapChain.images.data());

}

void VulkanClass::createImageViews() {

	swapChain.imageViews.resize(swapChain.images.size());

	for (size_t i = 0; i < swapChain.imageViews.size(); i++) {
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = swapChain.images[i];
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = swapChain.format.format;
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;

		if (vkCreateImageView(logicalDevice, &imageViewInfo, nullptr, &swapChain.imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Image View\n");
		}
	}

}

void VulkanClass::recreateSwapChain() {

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(logicalDevice);

	for (size_t i = 0; i < swapChain.framebuffers.size(); i++) {
		vkDestroyFramebuffer(logicalDevice, swapChain.framebuffers[i], nullptr);
	}
	for (size_t i = 0; i < swapChain.imageViews.size(); i++) {
		vkDestroyImageView(logicalDevice, swapChain.imageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(logicalDevice, swapChain.__swapChain, nullptr);

	if (!checkSwapChainSupport(physicalDevice)) {
		throw std::runtime_error("SwapChain not Supported\n");
	}

	createSwapChain();
	createImageViews();
	createDepthResources();
	createFramebuffers();

}

void VulkanClass::createRenderPass() {

	VkAttachmentDescription attachmentInfo{};
	attachmentInfo.format = swapChain.format.format;
	attachmentInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkFormat depthFormat = findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference attachmentRef{};
	attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentRef.attachment = 0;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependencies{};
	dependencies.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies.dstSubpass = 0;
	dependencies.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies.srcAccessMask = 0;
	dependencies.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::vector<VkAttachmentDescription> attachments = { attachmentInfo, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependencies;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Render Pass\n");
	}

}

void VulkanClass::createDescriptorSetLayout() {

	VkDescriptorSetLayoutBinding transformLayoutBinding{};
	transformLayoutBinding.binding = 0;
	transformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	transformLayoutBinding.descriptorCount = 1;
	transformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo transformLayoutInfo{};
	transformLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	transformLayoutInfo.bindingCount = 1;
	transformLayoutInfo.pBindings = &transformLayoutBinding;

	if (vkCreateDescriptorSetLayout(logicalDevice, &transformLayoutInfo, nullptr, &transformDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Transform Descriptor Set layout\n");
	}

	std::vector<VkDescriptorSetLayoutBinding> computeLayoutBindings(3);
	computeLayoutBindings[0].binding = 0;
	computeLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	computeLayoutBindings[0].descriptorCount = 1;
	computeLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayoutBindings[1].binding = 1;
	computeLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeLayoutBindings[1].descriptorCount = 1;
	computeLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayoutBindings[2].binding = 2;
	computeLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeLayoutBindings[2].descriptorCount = 1;
	computeLayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo computeLayoutInfo{};
	computeLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	computeLayoutInfo.bindingCount = 3;
	computeLayoutInfo.pBindings = computeLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &computeLayoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Compute Descriptor Set Layout\n");
	}
}

uint32_t VulkanClass::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");

}

void VulkanClass::createTransformBuffer(VkDeviceSize bufferSize) {

	transformBuffer.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	transformBufferMemory.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	transformBufferMap.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &transformBuffer[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed To create Transform Uniform Buffer\n");

		VkMemoryRequirements memreq;
		vkGetBufferMemoryRequirements(logicalDevice, transformBuffer[i], &memreq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memreq.size;
		allocInfo.memoryTypeIndex = findMemoryType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &transformBufferMemory[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to Allocate Transform Uniform Buffer Memory\n6");

		vkBindBufferMemory(logicalDevice, transformBuffer[i], transformBufferMemory[i], 0);

		vkMapMemory(logicalDevice, transformBufferMemory[i], 0, bufferSize, 0, &transformBufferMap[i]);
	}

}

void VulkanClass::createDescriptorPools() {

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &uniformDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Uniform Descriptor Pool\n");
	}

	std::vector<VkDescriptorPoolSize> poolSizes(2);
	poolSizes[0] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT * 2);

	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &computeDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Compute Descriptor Pool\n");
	}

}

void VulkanClass::createTransformDescriptorSet() {

	std::vector<VkDescriptorSetLayout> layouts(static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT), transformDescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = uniformDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	transformDescriptorSet.resize(static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT));

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, transformDescriptorSet.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Transform Descriptor Set\n");
	}

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = transformBuffer[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(transform);

		VkWriteDescriptorSet transformWrite{};
		transformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		transformWrite.dstSet = transformDescriptorSet[i];
		transformWrite.dstBinding = 0;
		transformWrite.dstArrayElement = 0;
		transformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		transformWrite.descriptorCount = 1;
		transformWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(logicalDevice, 1, &transformWrite, 0, nullptr);
	}

}

void VulkanClass::createGraphicsPipeline() {

	basicShader = new Shader("shader", logicalDevice);

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Particle);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 1;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Particle, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 2;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Particle, normal);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain.extent.width);
	viewport.height = static_cast<float>(swapChain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissorRect{};
	scissorRect.extent = swapChain.extent;
	scissorRect.offset = { 0,0 };

	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pScissors = &scissorRect;
	viewportStateInfo.pViewports = &viewport;

	VkPipelineRasterizationStateCreateInfo rasterInfo{};
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterInfo.depthClampEnable = VK_FALSE;
	rasterInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterInfo.lineWidth = 3.0f;
	rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.depthBiasEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampleInfo{};
	multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlend{};
	colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlend.blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo colorBlendGlobal{};
	colorBlendGlobal.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendGlobal.logicOpEnable = VK_FALSE;
	colorBlendGlobal.attachmentCount = 1;
	colorBlendGlobal.pAttachments = &colorBlend;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &transformDescriptorSetLayout;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Pipeline Layout\n");
	}


	//CREATING GRAPHICS PIPELINE

	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineInfo.stageCount = basicShader->shaderStageInfos.size();
	graphicsPipelineInfo.pStages = basicShader->shaderStageInfos.data();
	graphicsPipelineInfo.pColorBlendState = &colorBlendGlobal;
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pMultisampleState = &multisampleInfo;
	graphicsPipelineInfo.pRasterizationState = &rasterInfo;
	graphicsPipelineInfo.pDepthStencilState = &depthStencil;
	graphicsPipelineInfo.pViewportState = &viewportStateInfo;
	graphicsPipelineInfo.layout = pipelineLayout;
	graphicsPipelineInfo.renderPass = renderPass;
	graphicsPipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Graphics Pipeline\n");
	}

}

void VulkanClass::createFramebuffers() {

	swapChain.framebuffers.resize(swapChain.imageViews.size());

	for (size_t i = 0; i < swapChain.framebuffers.size(); i++) {
		std::vector<VkImageView> attachments = {
			swapChain.imageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChain.extent.width;
		framebufferInfo.height = swapChain.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChain.framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Framebuffer\n");
		}

	}

}

void VulkanClass::createCommandPool() {

	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = QueueFamilyIndex.graphicsFamily;

	if (vkCreateCommandPool(logicalDevice, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Command Pool\n");
	}

}

void VulkanClass::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) {
	
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Being Recording Command Buffer\n");
	}

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = swapChain.framebuffers[imageIndex];
	renderPassBeginInfo.renderArea.offset = { 0,0 };
	renderPassBeginInfo.renderArea.extent = swapChain.extent;

	std::vector<VkClearValue> clearValues(2);
	clearValues[0].color = { {0.2f, 0.3f, 0.3f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };


	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();
	
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain.extent.width);
	viewport.height = static_cast<float>(swapChain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	//vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissorRect{};
	scissorRect.extent = swapChain.extent;
	scissorRect.offset = { 0,0 };
	//vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &posBuffer[1], offsets);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &transformDescriptorSet[currentFrame], 0, nullptr);

	vkCmdDraw(commandBuffer, NUM_PARTICLES*15.0, 1, 0, 0);
	//vkCmdDraw(commandBuffer, 36, 1000, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Record Command Buffer\n");
	}

}

void VulkanClass::createCommandBuffer() {

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	commandBuffer.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Allocate Command Buffer\n");
		}
	}

}

void VulkanClass::createSyncObjects() {

	imageAvailableSemaphore.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	renderFinishedSempahore.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	inFlightFence.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	computeFinishedSemaphores.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	computeInFlightFences.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSempahore[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFence[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Sync Objects\n");
		}

		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to Create Compute Sync Objects\n");
		}
	}

}

void VulkanClass::draw(uint32_t imageIndex) {

	uint32_t index;

	VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain.__swapChain, UINT32_MAX, imageAvailableSemaphore[imageIndex], VK_NULL_HANDLE, &index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		std::cout << "NO WORK SUBMITTED\n";
		return;
	}

	//std::cout << "WORK SUBMITTED\n";

	vkResetFences(logicalDevice, 1, &inFlightFence[imageIndex]);

	vkResetCommandBuffer(commandBuffer[imageIndex], 0);

	recordCommandBuffer(commandBuffer[imageIndex], index, imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	VkSemaphore waitSemaphores[] = { computeFinishedSemaphores[imageIndex], imageAvailableSemaphore[imageIndex] };
	VkSemaphore signalSemaphores[] = { renderFinishedSempahore[imageIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence[imageIndex]) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Submit Draw Command\n");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	
	VkSwapchainKHR swapChains[] = { swapChain.__swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &index;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
		//std::cout<<"recreated swapchain\n";
	}

}

void VulkanClass::updateTransform() {

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		memcpy(transformBufferMap[i], &transform, sizeof(transform));
	}

}

float randomFloat()
{
	return (float)(rand()) / (float)(RAND_MAX);
}

void VulkanClass::createPosBuffer() {

	VkBufferCreateInfo posBufferCreateInfo{};
	posBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	posBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	posBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	posBufferCreateInfo.size = sizeof(float) * NUM_PARTICLES;

	posBuffer.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	posBufferMemory.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	posBufferMap.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	computeUniformBuffer.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	computeUniformBufferMemory.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	computeUniformBufferMap.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		if (i == 1) {
			posBufferCreateInfo.size = sizeof(Particle) * NUM_PARTICLES * 15.0;
		}

		if (vkCreateBuffer(logicalDevice, &posBufferCreateInfo, nullptr, &posBuffer[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to Create Pos Buffer\n");
		}

		VkMemoryRequirements memreq;
		vkGetBufferMemoryRequirements(logicalDevice, posBuffer[i], &memreq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memreq.size;
		allocInfo.memoryTypeIndex = findMemoryType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		std::cout << memreq.size << "\n";

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &posBufferMemory[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to Allocate Transform Uniform Buffer Memory\n6");

		vkBindBufferMemory(logicalDevice, posBuffer[i], posBufferMemory[i], 0);

		vkMapMemory(logicalDevice, posBufferMemory[i], 0, memreq.size, 0, &posBufferMap[i]);

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(ComputeUniforms);
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &computeUniformBuffer[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed To create Transform Uniform Buffer\n");

		vkGetBufferMemoryRequirements(logicalDevice, computeUniformBuffer[i], &memreq);
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memreq.size;
		allocInfo.memoryTypeIndex = findMemoryType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &computeUniformBufferMemory[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to Allocate Transform Uniform Buffer Memory\n6");

		vkBindBufferMemory(logicalDevice, computeUniformBuffer[i], computeUniformBufferMemory[i], 0);

		vkMapMemory(logicalDevice, computeUniformBufferMemory[i], 0, sizeof(ComputeUniforms), 0, &computeUniformBufferMap[i]);
	}

	VkBuffer stagingBuffer = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	VkBufferCreateInfo stagingBufferCreateInfo{};
	stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	stagingBufferCreateInfo.size = sizeof(float) * NUM_PARTICLES;

	std::vector<float> field;
	std::vector<Particle> particles;

	for (size_t i = 0; i < NUM_PARTICLES; i++)  {

		int cell_x = (i % gridSize) - gridSize/2;
		int cell_y = (i / gridSize2) - gridSize/2;
		int cell_z = (i / gridSize)%gridSize - gridSize/2;

		float dist = sqrt(cell_x * cell_x + cell_y * cell_y + cell_z * cell_z);

		float fieldStrength = 1.0f;

		if (dist > 6.0) {
			fieldStrength = 0.0f;
		}

		field.push_back(fieldStrength);
	}

	//for (size_t i = 0; i < NUM_PARTICLES; i++) {
	//	
	//	std::cout<<field[i]<<" ";

	//	if (i % gridSize == 0) {
	//		std::cout << "|";
	//	}
	//	if (i % gridSize2 == 0) {
	//		std::cout << "\n";
	//	}

	//}

	for (size_t i = 0; i < NUM_PARTICLES * 15.0; i++) {
		Particle part;

		part.pos = glm::vec4(1.0f);
		part.normal = glm::vec4(-1.0f);

		particles.push_back(part);
	}

	std::cout << "Pos Buffer Created\n";

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {

		if (i == 1) {
			stagingBufferCreateInfo.size = sizeof(Particle) * NUM_PARTICLES * 15.0;
		}

		vkCreateBuffer(logicalDevice, &stagingBufferCreateInfo, nullptr, &stagingBuffer);

		VkMemoryRequirements memreq;
		vkGetBufferMemoryRequirements(logicalDevice, stagingBuffer, &memreq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memreq.size;
		allocInfo.memoryTypeIndex = findMemoryType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &stagingBufferMemory);

		vkBindBufferMemory(logicalDevice, stagingBuffer, stagingBufferMemory, 0);

		void* data = nullptr;

		if (i == 0) {
			vkMapMemory(logicalDevice, stagingBufferMemory, 0, sizeof(float) * NUM_PARTICLES, 0, &data);
			memcpy(data, field.data(), (size_t)(sizeof(float) * NUM_PARTICLES));
			vkUnmapMemory(logicalDevice, stagingBufferMemory);
		}
		else {
			vkMapMemory(logicalDevice, stagingBufferMemory, 0, sizeof(Particle) * NUM_PARTICLES * 15.0, 0, &data);
			memcpy(data, particles.data(), (size_t)(sizeof(Particle) * NUM_PARTICLES * 15.0));
			vkUnmapMemory(logicalDevice, stagingBufferMemory);
		}

		VkCommandBufferAllocateInfo cmdAllocInfo{};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdAllocInfo.commandPool = commandPool;
		cmdAllocInfo.commandBufferCount = 1;

		VkCommandBuffer copyCommandBuffer;
		vkAllocateCommandBuffers(logicalDevice, &cmdAllocInfo, &copyCommandBuffer);

		VkCommandBufferBeginInfo copyBeginInfo{};
		copyBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		copyBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(copyCommandBuffer, &copyBeginInfo);

		VkBufferCopy copyRegion{};
		if (i==0)
			copyRegion.size = sizeof(float) * NUM_PARTICLES;
		else 
			copyRegion.size = sizeof(Particle) * NUM_PARTICLES * 15.0;
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		vkCmdCopyBuffer(copyCommandBuffer, stagingBuffer, posBuffer[i], 1, &copyRegion);

		vkEndCommandBuffer(copyCommandBuffer);

		VkSubmitInfo copySubmitInfo{};
		copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		copySubmitInfo.commandBufferCount = 1;
		copySubmitInfo.pCommandBuffers = &copyCommandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &copySubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &copyCommandBuffer);
	}

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

}

void VulkanClass::createComputeDescriptorSet() {

	std::vector<VkDescriptorSetLayout> layouts(static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT), computeDescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = computeDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	computeDescriptorSets.resize(static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT));

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, computeDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Compute Descriptor Set\n");
	}

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {

		std::vector<VkWriteDescriptorSet> descriptorWrites(3);
		
		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = computeUniformBuffer[i];
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(ComputeUniforms);

		descriptorWrites[0] = {};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].dstSet = computeDescriptorSets[i];
		descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

		VkDescriptorBufferInfo shaderStoragePrevFrame{};
		shaderStoragePrevFrame.buffer = posBuffer[0];
		shaderStoragePrevFrame.offset = 0;
		shaderStoragePrevFrame.range = sizeof(float) * NUM_PARTICLES;

		descriptorWrites[1] = {};
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].dstSet = computeDescriptorSets[i];
		descriptorWrites[1].pBufferInfo = &shaderStoragePrevFrame;

		VkDescriptorBufferInfo shaderStorageNextFrame{};
		shaderStorageNextFrame.buffer = posBuffer[1];
		shaderStorageNextFrame.offset = 0;
		shaderStorageNextFrame.range = sizeof(Particle) * NUM_PARTICLES * 15.0;

		descriptorWrites[2] = {};
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].dstSet = computeDescriptorSets[i];
		descriptorWrites[2].pBufferInfo = &shaderStorageNextFrame;

		vkUpdateDescriptorSets(logicalDevice, 3, descriptorWrites.data(), 0, 0);

	}

}

void VulkanClass::createComputePipeline() {

	VkPipelineLayoutCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &computeDescriptorSetLayout;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Compute Pipeline Layout\n");
	}

	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.layout = computePipelineLayout;
	computePipelineInfo.stage = basicShader->computeShaderStageInfo;

	if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Compute Pipeline\n");
	}

}

void VulkanClass::updateCompute() {

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		memcpy(computeUniformBufferMap[i], &computeUniform, sizeof(ComputeUniforms));
	}

}

void VulkanClass::recordComputeCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Begin Recording Compute Command Buffer\n");
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSets[imageIndex], 0, 0);

	vkCmdDispatch(commandBuffer, NUM_PARTICLES, 1, 1);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Record Compute Command Buffer\n");
	}

}

void VulkanClass::dispatch(uint32_t imageIndex) {

	vkResetFences(logicalDevice, 1, &computeInFlightFences[imageIndex]);

	vkResetCommandBuffer(commandBuffer[imageIndex], 0);
	recordComputeCommandBuffer(commandBuffer[imageIndex], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore signalSemaphores[] = { computeFinishedSemaphores[imageIndex] };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, computeInFlightFences[imageIndex]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Submit Compute Command\n");
	}

}

void VulkanClass::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(logicalDevice, image, imageMemory, 0);
}

VkImageView VulkanClass::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

VkCommandBuffer VulkanClass::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanClass::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

bool VulkanClass::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanClass::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void VulkanClass::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

VkFormat VulkanClass::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");

}

void VulkanClass::createDepthResources() {

	VkFormat depthFormat = findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	createImage(swapChain.extent.width, swapChain.extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkImageView VulkanClass::createImageView(VkImage image, VkFormat format) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}