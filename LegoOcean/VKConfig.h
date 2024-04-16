#pragma once

#ifndef VK_CONFIG
	#define VK_CONFIG
#endif

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>

#include "Shaders.h"

struct Transform {
	glm::mat4 M;
	glm::mat4 V;
	glm::mat4 P;
};

struct Particle {
	glm::vec4 pos;
};

struct ComputeUniforms {
	float deltaTime;
};

struct QueueFamily {

	uint32_t graphicsFamily;
	uint32_t presentFamily;

};

struct SwapChainSupport {

	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

};

struct SwapChain {

	VkSwapchainKHR __swapChain;

	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	const int MAX_FRAMES_IN_FLIGHT = 2;;

	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	std::vector<VkFramebuffer> framebuffers;
 
	VkSurfaceFormatKHR findSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR findSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

};

class VulkanClass {

private:

	bool enableValidationLayers = true;
	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	QueueFamily QueueFamilyIndex;
	SwapChainSupport SwapChainDetails;

	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSurfaceKHR surface;
	GLFWwindow* window;

	SwapChain swapChain;

	VkDescriptorSetLayout transformDescriptorSetLayout;
	VkDescriptorPool uniformDescriptorPool;
	std::vector<VkDescriptorSet> transformDescriptorSet;
	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorPool computeDescriptorPool;
	std::vector<VkDescriptorSet> computeDescriptorSets;

	std::vector<VkBuffer> transformBuffer;
	std::vector<VkDeviceMemory> transformBufferMemory;
	std::vector<void*> transformBufferMap;

	std::vector<std::vector<VkBuffer>> posBuffer;
	std::vector<std::vector<VkDeviceMemory>> posBufferMemory;

	std::vector<VkBuffer> computeUniformBuffer;
	std::vector<VkDeviceMemory> computeUniformBufferMemory;
	std::vector<void*> computeUniformBufferMap;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkPipelineLayout computePipelineLayout;
	VkPipeline computePipeline;

	Shader* basicShader;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffer;

public:

	int NUM_PARTICLES = 10000;

	bool framebufferResized = false;

	std::vector<VkSemaphore> imageAvailableSemaphore;
	std::vector<VkSemaphore> renderFinishedSempahore;
	std::vector<VkFence> inFlightFence;

	std::vector<VkSemaphore> computeFinishedSemaphores;
	std::vector<VkFence> computeInFlightFences;

	Transform transform;
	ComputeUniforms computeUniform;

	VulkanClass();
	VulkanClass(GLFWwindow* win);
	~VulkanClass();

	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
	bool findQueueFamilies(VkPhysicalDevice device);
	bool checkSwapChainSupport(VkPhysicalDevice device);
	VkDevice getLogicalDevice() { return logicalDevice; }
	void draw(uint32_t imageIndex);
	void dispatch(uint32_t imageIndex);
	int getMaxFramesInFlight() { return swapChain.MAX_FRAMES_IN_FLIGHT; }
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	//void initVulkan();
	void createInstance();
	VkPhysicalDevice findPhysicalDevice();
	void createLogicalDevice();
	void createSurface();

	void createSwapChain();
	void createImageViews();
	void recreateSwapChain();

	void createDescriptorSetLayout();
	void createTransformBuffer(VkDeviceSize bufferSize);
	void createDescriptorPools();
	void createTransformDescriptorSet();

	void createPosBuffer();
	void createComputeDescriptorPool();
	void createComputeDescriptorSet();

	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();

	void createComputePipeline();
	
	void createCommandPool();
	void createCommandBuffer();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
	void recordComputeCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void createSyncObjects();

	void updateTransform();
	void updateCompute();

	void createVertexBuffer();

};