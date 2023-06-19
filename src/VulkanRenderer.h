/*
author: Pyrrha Tocquet
date: 22/05/23
desc: Manages the rendering flow and logic. "Heart of the Renderer"
*/
#pragma once

#include "Defs.h"
#include <GLFW/glfw3.h>
#include "VulkanTools.h"
#include "glm/glm.hpp"
#include <iostream>
#include <optional>
#include "spirv_reflect.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanScene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "VulkanRenderPass.h"
#include "MainRenderPass.h"
#include "ShadowRenderPass.h"
#include "ShadowCascadeRenderPass.h"
#include "Camera.h"
#include "VulkanPipeline.h"



/* CONSTANTS */
const uint32_t DESCRIPTOR_SET_LAYOUT_BINDINGS = 3;
const uint32_t PUSH_CONSTANTS_COUNT = 1;


struct TexturedMesh;

class VulkanScene;
class VulkanRenderPass;
class MainRenderPass;
class ShadowRenderPass;

class VulkanRenderer
{
private:
	VulkanContext* m_context = nullptr;

	vk::Device m_device = VK_NULL_HANDLE;
	vma::Allocator m_allocator;

	//COMMAND BUFFER
	std::vector<vk::CommandBuffer> m_commandBuffers;

	vk::SampleCountFlagBits m_msaaSampleCount = vk::SampleCountFlagBits::e1;
	
	//RENDER PASS
	std::vector<VulkanRenderPass*> m_renderPasses;
	ShadowCascadeRenderPass* m_shadowPass;
	MainRenderPass* m_mainPass;

	//RENDERING FLOW
	uint32_t m_currentFrame = 0;

	//SYNC
	std::vector<vk::Fence> m_inFlightFences;
	std::vector<vk::Semaphore> m_imageAvailableSemaphores;
	std::vector<vk::Semaphore> m_renderFinishedSemaphores;


	/*-------------------------------------------*/

	uint32_t m_mipLevels = 10;
	/*-------------------------------------------*/

	std::vector<VulkanScene*> m_scenes;
	std::vector<Entity*> m_entities;

	Camera* m_camera;
	bool m_shouldStopRendering = false;
public:
	VulkanRenderer(VulkanContext* context);
	void mainloop();
	void drawFrame();
	void addScene(VulkanScene* vulkanScene);
	~VulkanRenderer();

	void recreateSwapchainSizedObjects();
	void addPipeline(VulkanPipeline* pipeline);

	void registerEntity(Entity* entity);
private:

	//RENDER PASS
	void createRenderPasses();

	//FRAMEBUFFERS
	void createFramebuffers();


	void cleanSwapchainSizedObjects();

	//COMMAND BUFFERS
	void createCommandBuffers();
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex);


	//SYNCHRONISATION
	void createSyncObjects();

	//EXECUTION FLOW
	bool present(vk::Semaphore* signalSemaphores, uint32_t imageIndex);
	void updateUniformBuffers(uint32_t imageIndex);

	//INPUTS
	void manageInput();

	//ENTITIES
	void updateEntities();


};

