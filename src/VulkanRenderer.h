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
#include "Camera.h"




/* CONSTANTS */
const uint32_t DESCRIPTOR_SET_LAYOUT_BINDINGS = 3;
const uint32_t PUSH_CONSTANTS_COUNT = 1;
const uint32_t MAX_TEXTURE_COUNT = 4096;

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

	//DESCRIPTORS
	vk::DescriptorSetLayout m_mainDescriptorSetLayout = VK_NULL_HANDLE;
	vk::DescriptorSetLayout m_shadowDescriptorSetLayout = VK_NULL_HANDLE;
	vk::DescriptorPool m_mainDescriptorPool, m_shadowDescriptorPool = VK_NULL_HANDLE;

	//PUSH CONSTANTS
	std::vector<vk::PushConstantRange> m_pushConstantRanges;

	vk::SampleCountFlagBits m_msaaSampleCount = vk::SampleCountFlagBits::e1;
	
	//GRAPHICS PIPELINE
	std::vector<VulkanPipeline> m_pipelines;
	uint32_t m_currentPipelineId = 0;
	vk::PipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VulkanPipeline m_shadowPipeline;

	//RENDER PASS
	std::vector<VulkanRenderPass*> m_renderPasses;

	//RENDERING FLOW
	uint32_t m_currentFrame = 0;

	//SYNC
	std::vector<vk::Fence> m_inFlightFences;
	std::vector<vk::Semaphore> m_imageAvailableSemaphores;
	std::vector<vk::Semaphore> m_renderFinishedSemaphores;

	/*-------------------------------------------*/
	std::vector<vk::Buffer> m_uniformBuffers;
	std::vector<vma::Allocation> m_uniformBuffersAllocations;
	vk::Sampler m_textureSampler = VK_NULL_HANDLE;
	uint32_t m_mipLevels = 10;
	VulkanImage* m_defaultTexture, *m_defaultNormalMap = nullptr;

	std::vector<vk::DescriptorSet> m_shadowDescriptorSets;
	/*-------------------------------------------*/

	std::vector<VulkanScene*> m_scenes;
	std::vector<Entity*> m_entities;

	Camera* m_camera;
public:
	VulkanRenderer(VulkanContext* context);
	void mainloop();
	void drawFrame();
	void addScene(VulkanScene* vulkanScene);
	~VulkanRenderer();

	void recreateSwapchainSizedObjects();
	[[nodiscard]] VulkanPipeline createPipeline(PipelineInfo pipelineInfo);
	void addPipeline(VulkanPipeline pipeline);

	void registerEntity(Entity* entity);
private:
	//PIPELINE
	void createMainGraphicsPipeline(const char* vertShaderCodePath, const char* fragShaderCodePath);
	void createPipelineLayout();
	[[nodiscard]] vk::ShaderModule createShaderModule(std::vector<char>& shaderCode);
	void createShadowGraphicsPipeline(const char* vertShaderCodePath, const char* fragShaderCodePath);

	//RENDER PASS
	void createRenderPasses();

	//DESCRIPTORS
	void createDescriptorSetLayout();
	void createDescriptorPools();
	[[nodiscard]] std::vector<vk::DescriptorSet> createDescriptorSets(VulkanScene* scene);
	void createDescriptorObjects();
	void createDefaultTextures();
	void createShadowDescriptorSets();

	//PUSH CONSTANTS
	void createPushConstantRanges();

	//FRAMEBUFFERS
	void createFramebuffers();


	void cleanSwapchainSizedObjects();

	//COMMAND BUFFERS
	void createCommandBuffers();
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex);
	//TODO DO BETTER
	void createUniformBuffer();
	void createTextureSampler();


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

