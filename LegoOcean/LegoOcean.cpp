#include "VKConfig.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>

namespace win {
	int width = 3840;
	int height = 2160;
}

namespace camera {
	glm::vec3 pos = glm::vec3(0.0f, -5.0f, -300.0f);
	glm::vec3 fwd = glm::vec3(0.0f, 0.0f, 1.0f);
	float angle = 0;
	float Xangle = 0;
}

Transform transform;
ComputeUniforms computeUniform;

std::unique_ptr<VulkanClass> vk;

namespace hostSwapChain {
	uint32_t currentFrame = 0;
}


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}
	if (key == GLFW_KEY_A) {
		camera::pos -= glm::cross(camera::fwd, glm::vec3(0.0, 1.0, 0.0));
	}
	if (key == GLFW_KEY_D) {
		camera::pos += glm::cross(camera::fwd, glm::vec3(0.0, 1.0, 0.0));
	}
	if (key == GLFW_KEY_Q) {
		camera::pos += glm::vec3(0.0f, 1.0f, 0.0f);
	}
	if (key == GLFW_KEY_E) {
		camera::pos += glm::vec3(0.0f, -1.0f, 0.0f);
	}
	if (key == GLFW_KEY_W) {
		//camera::pos += glm::vec3(0.0f, 0.0f, 0.1f);
		camera::pos += camera::fwd;
	}
	if (key == GLFW_KEY_S) {
		//camera::pos += glm::vec3(0.0f, 0.0f, -0.1f);
		camera::pos -= camera::fwd;
	}
	if (key == GLFW_KEY_LEFT) {
		camera::angle += 0.1f;
		camera::fwd = glm::vec3(sin(camera::angle), camera::fwd.y, cos(camera::angle));
	}
	if (key == GLFW_KEY_RIGHT) {
		camera::angle -= 0.1f;
		camera::fwd = glm::vec3(sin(camera::angle), camera::fwd.y, cos(camera::angle));
	}
	if (key == GLFW_KEY_UP) {
		camera::Xangle -= 0.1f;
		camera::fwd = glm::vec3(camera::fwd.x, sin(camera::Xangle), cos(camera::Xangle));
	}
	if (key == GLFW_KEY_DOWN) {
		camera::Xangle += 0.1f;
		camera::fwd = glm::vec3(camera::fwd.x, sin(camera::Xangle), cos(camera::Xangle));
	}
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	
	vk->framebufferResized = true;

}

void display() {

	vkWaitForFences(vk->getLogicalDevice(), 1, &vk->inFlightFence[hostSwapChain::currentFrame], VK_TRUE, UINT32_MAX);

	vk->dispatch(hostSwapChain::currentFrame);

	vkWaitForFences(vk->getLogicalDevice(), 1, &vk->computeInFlightFences[hostSwapChain::currentFrame], VK_TRUE, UINT64_MAX);

	vk->draw(hostSwapChain::currentFrame);

	hostSwapChain::currentFrame = (hostSwapChain::currentFrame + 1) % vk->getMaxFramesInFlight();
}

void idle() {

	transform.M = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * glm::mat4(1.0f);
	transform.V = glm::lookAt(camera::pos, camera::pos + camera::fwd, glm::vec3(0.0f, 1.0f, 0.0f));
	transform.P = glm::perspective(glm::radians(45.0f), win::width / (float)win::height, 0.1f, 1000.0f);
	vk->transform = transform;

	vk->updateTransform();
	
	computeUniform.deltaTime = glfwGetTime() / 1000.0;
	vk->computeUniform = computeUniform;

	vk->updateCompute();

}

int main() {

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(win::width, win::height, "Lego Ocean", 0, nullptr);

	vk.reset(new VulkanClass (window));
	vk->createTransformBuffer(sizeof(transform));
	vk->createTransformDescriptorSet();
	vk->createPosBuffer();
	vk->createComputeDescriptorSet();

	glfwSetKeyCallback(window, keyboardCallback);
	glfwSetWindowSizeCallback(window, windowResizeCallback);

	while (!glfwWindowShouldClose(window)) {

		idle();
		display();

		glfwPollEvents();

	}

	vkDeviceWaitIdle(vk->getLogicalDevice());

	vk.reset();

	return 0;

}