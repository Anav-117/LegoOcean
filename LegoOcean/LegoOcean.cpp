#include "VKConfig.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "TraingleTable.h"
#include <iostream>

namespace win {
	int width = 3840;
	int height = 2160;
}

float t = 0;

namespace camera {
	glm::vec3 pos = glm::vec3(40.0f, 40.0f, -160.0f);
	glm::vec3 fwd = glm::vec3(0.0f, 0.0f, 1.0f);
	float angle = 0;
	float Xangle = 0;
}

namespace field {
	int fieldMode = 0;
	bool first = true;
}

bool CPU = false;

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
	if (key == GLFW_KEY_0 && action == GLFW_RELEASE) {
		field::fieldMode = 0;
		field::first = true;
		transform.wave = 0;
	}
	if (key == GLFW_KEY_1) {
		field::fieldMode = 1;
		transform.wave = 0;
	}
	if (key == GLFW_KEY_2 && action == GLFW_RELEASE) {
		field::fieldMode = 2;
		field::first = true;
		transform.wave = 0;
	}
	if (key == GLFW_KEY_3) {
		field::fieldMode = 3;
		transform.wave = 1;
	}
	if (key == GLFW_KEY_4) {
		field::fieldMode = 4;
		field::first = true;
		transform.wave = 0;
	}
	if (key == GLFW_KEY_5 && action == GLFW_RELEASE) {
		CPU = !CPU;

		std::vector<Particle> vertices;

		for (int i = 0; i < vk->NUM_PARTICLES; i++) {
			for (int j = 0; j < 15; j++) {
				Particle vert;

				vert.pos = glm::vec4(0.0f);
				vert.normal = glm::vec4(0.0f);

				vertices.push_back(vert);
			}
		}

		memcpy(vk->posBufferMap[1], vertices.data(), sizeof(Particle) * vk->NUM_PARTICLES * 15.0);

		transform.wave = 0;
	}
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	
	vk->framebufferResized = true;

}

void diagnostics() {

	Particle* buffer = reinterpret_cast<Particle*>(vk->posBufferMap[1]);

	for (int i = 0; i < vk->NUM_PARTICLES; i++) {

		//if (buffer[i].pos[3] == 0) {
		//	std::cout << "0 ";
		//}
		//else {
		//	std::cout << "1 ";
		//}
		/*std::cout << buffer[i].normal[3] << " ";

		if ((i) % 10 == 0) {
			std::cout << "|";
		}
		if ((i) % 100 == 0) {
			std::cout << "\n";
		}*/

		std::cout << buffer[i].pos.x << " | "  << buffer[i].pos.y << " | " << buffer[i].pos.z << "\n";
	}

}

void advectField() {

	std::vector<float> data;
	float* buffer = reinterpret_cast<float*>(vk->posBufferMap[0]);
	std::vector<Particle> vertices;
	float t_before;

	switch (field::fieldMode) {
	case 0:
		if (!field::first) { break; }
		for (size_t i = 0; i < vk->NUM_PARTICLES; i++) {

			int cell_x = (i % vk->gridSize) - vk->gridSize / 2;
			int cell_y = (i / vk->gridSize2) - vk->gridSize / 2;
			int cell_z = (i / vk->gridSize) % vk->gridSize - vk->gridSize / 2;

			float dist = sqrt(cell_x * cell_x + cell_y * cell_y + cell_z * cell_z);

			float fieldStrength = 1.0f;

			if (dist > 4) {
				fieldStrength = 0.0f;
			}

			data.push_back(fieldStrength);
		}

		field::first = false;
		memcpy(vk->posBufferMap[0], data.data(), sizeof(float) * vk->NUM_PARTICLES);
		break;
	case 1:
		for (size_t i = 0; i < vk->NUM_PARTICLES; i++) {

			int cell_x = (i % vk->gridSize) - vk->gridSize/2;
			int cell_y = (i / vk->gridSize2) - vk->gridSize/2;
			int cell_z = (i / vk->gridSize) % vk->gridSize - vk->gridSize/2;

			float dist = sqrt(cell_x * cell_x + cell_y * cell_y + cell_z * cell_z);

			float fieldStrength = 1.0f;

			if (dist > 4 * abs(sin(glfwGetTime()))) {
				fieldStrength = 0.0f;
			}

			data.push_back(fieldStrength);
		}

		memcpy(vk->posBufferMap[0], data.data(), sizeof(float) * vk->NUM_PARTICLES);
		break;
	case 2:
		if (!field::first) { break; }
		for (size_t i = 0; i < vk->NUM_PARTICLES; i++) {

			float fieldStrength = 0.0;

			float random = ((float)rand() / float(RAND_MAX));
			if (random > 0.3) {
				fieldStrength = 1.0f;
			}

			data.push_back(fieldStrength);
		}

		field::first = false;
		memcpy(vk->posBufferMap[0], data.data(), sizeof(float) * vk->NUM_PARTICLES);
		break;
	case 3: 
		for (size_t i = 0; i < vk->NUM_PARTICLES; i++) {

			int cell_x = (i % vk->gridSize) - vk->gridSize / 2;
			int cell_y = (i / vk->gridSize2) - vk->gridSize / 2;
			int cell_z = (i / vk->gridSize) % vk->gridSize - vk->gridSize / 2;

			float fieldStrength = 0.0;

			if (cell_z < (sin(cell_x + glfwGetTime()*3.0) + cos(cell_y + glfwGetTime() * 3.0))) {
				fieldStrength = 1.0;
			}


			data.push_back(fieldStrength);
		}

		memcpy(vk->posBufferMap[0], data.data(), sizeof(float) * vk->NUM_PARTICLES);
		break;
	case 4:
		if (field::first) {
			for (size_t i = 0; i < vk->NUM_PARTICLES; i++) {

				float fieldStrength = 0.0;

				data.push_back(fieldStrength);
			}

			field::first = false;
			memcpy(vk->posBufferMap[0], data.data(), sizeof(float) * vk->NUM_PARTICLES);
		}
		else {

			for (size_t i = 0; i < vk->NUM_PARTICLES; i++) {

				int cell_x = (i % vk->gridSize) - vk->gridSize / 2;
				int cell_y = (i / vk->gridSize2) - vk->gridSize / 2;
				int cell_z = (i / vk->gridSize) % vk->gridSize - vk->gridSize / 2;

				float fieldStrength = 0.0;
				 
				if (buffer[i] == 1.0) {
					fieldStrength = 1.0;
				}
				else {
					float random = ((float)rand() / float(RAND_MAX));
					if (random > 0.999) {
						fieldStrength = 1.0f;
					}
				}

				if (abs(cell_x) >= vk->gridSize/2-1 || abs(cell_y) >= vk->gridSize / 2 - 1 || abs(cell_z) >= vk->gridSize / 2 - 1) {
					fieldStrength = 0.0;
				}

				data.push_back(fieldStrength);
			}

			field::first = false;
			memcpy(vk->posBufferMap[0], data.data(), sizeof(float) * vk->NUM_PARTICLES);
		}
		break;
	case 5:
		CPU = !CPU;
		break;
	default:
		//pass
		break;
	}

	if (CPU) {
		t_before = glfwGetTime();
		for (int i = 0; i < vk->NUM_PARTICLES; i++) {
			for (int j = 0; j < 15; j++) {
				Particle vert = march(i, j, buffer);

				vertices.push_back(vert);
			}
		}

		//std::cout << "CPU TIME - " << glfwGetTime() - t_before << "\n";

		memcpy(vk->posBufferMap[1], vertices.data(), sizeof(Particle) * vk->NUM_PARTICLES * 15.0);
	}

}

void display() {

	vkWaitForFences(vk->getLogicalDevice(), 1, &vk->inFlightFence[hostSwapChain::currentFrame], VK_TRUE, UINT32_MAX);

	float t_before = glfwGetTime();

	vk->dispatch(hostSwapChain::currentFrame);

	vkWaitForFences(vk->getLogicalDevice(), 1, &vk->computeInFlightFences[hostSwapChain::currentFrame], VK_TRUE, UINT64_MAX);

	if (!CPU) {
		//std::cout << "GPU TIME - " << glfwGetTime() - t_before << "\n";
	}

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
	if (CPU)
		computeUniform.fieldMode = 5;
	else
		computeUniform.fieldMode = 0;

	vk->computeUniform = computeUniform;

	vk->updateCompute();

	advectField();

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

		//diagnostics();

		glfwPollEvents();

	}

	vkDeviceWaitIdle(vk->getLogicalDevice());

	vk.reset();

	return 0;

}