#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
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


/* CONSTANTS */
const uint32_t DESCRIPTOR_SET_LAYOUT_BINDINGS = 2;
const uint32_t PUSH_CONSTANTS_COUNT = 1;
const uint32_t MAX_TEXTURE_COUNT = 2048;

const bool m_enableMSAA = false;


//TODO Organise better
constexpr std::array<vk::ClearValue, 2> setClearColors() {
	constexpr vk::ClearColorValue CLEAR_COLOR = vk::ClearColorValue{.float32= std::array<float, 4>{.8f, .8f, .8f, 1.0f}};
	constexpr vk::ClearDepthStencilValue CLEAR_DEPTH = vk::ClearDepthStencilValue({ 1.0f, 0 });
	std::array<vk::ClearValue, 2> clearValues{};
	clearValues[0].setColor(CLEAR_COLOR);
	clearValues[1].setDepthStencil(CLEAR_DEPTH);
	return clearValues;
}
constexpr std::array<vk::ClearValue, 2> CLEAR_VALUES = setClearColors();


struct TexturedMesh;
struct Model;

struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
};

struct ModelPushConstant {
	glm::mat4 model;
	glm::int32 textureId;
	glm::float32 time;
	glm::vec2 data;
};

struct PipelineInfo {
	const char* vertPath;
	const char* fragPath;
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlags cullmode = vk::CullModeFlagBits::eBack;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	float lineWidth = 1.0f;
	vk::Bool32 depthTestEnable = VK_TRUE;
	vk::Bool32 depthWriteEnable = VK_TRUE;
};

struct VulkanPipeline {
	PipelineInfo pipelineInfo;
	vk::Pipeline pipeline;
};

struct CameraCoords {
	glm::vec3 pitchYawRoll = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 cameraPos = glm::vec3(2.5f, 0.0, 1.0f);

	glm::vec3 getDirection() {
		glm::vec3 direction;
		direction.x = cos(glm::radians(pitchYawRoll.x)) * cos(glm::radians(pitchYawRoll.y));
		direction.z = sin(glm::radians(pitchYawRoll.x));
		direction.y = -cos(glm::radians(pitchYawRoll.x)) * sin(glm::radians(pitchYawRoll.y));
		direction = glm::normalize(direction);
		return direction;
	}

};

class VulkanScene;

class VulkanRenderer
{
private:
	VulkanContext* m_context = nullptr;

	vk::Device m_device = VK_NULL_HANDLE;
	vma::Allocator m_allocator;

	//FRAMEBUFFER
	VulkanImage *m_colorAttachment, *m_depthAttachment = nullptr;
	std::vector<vk::Framebuffer> m_framebuffers;

	//COMMAND BUFFER
	std::vector<vk::CommandBuffer> m_commandBuffers;

	//DESCRIPTORS
	vk::DescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	vk::DescriptorPool m_descriptorPool = VK_NULL_HANDLE;

	//PUSH CONSTANTS
	std::vector<vk::PushConstantRange> m_pushConstantRanges;

	vk::SampleCountFlagBits m_msaaSampleCount = vk::SampleCountFlagBits::e1;
	
	//GRAPHICS PIPELINE
	std::vector<VulkanPipeline> m_pipelines;
	uint32_t m_currentPipelineId = 0;
	vk::PipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	//RENDER PASS
	vk::RenderPass m_mainRenderPass = VK_NULL_HANDLE;
	vk::Format m_depthFormat = vk::Format::eUndefined; //use findDepthFormat instead of reading it directly

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
	uint32_t m_mipLevels = 1;
	VulkanImage* m_defaultTexture = nullptr;
	/*-------------------------------------------*/

	std::vector<VulkanScene*> m_scenes;
	CameraCoords m_camera;

	bool m_shouldRecreateSwapchain = false;

public:
	VulkanRenderer(VulkanContext* context);
	void mainloop();
	void drawFrame();
	void addScene(VulkanScene* vulkanScene);
	~VulkanRenderer();

	void recreateSwapchainSizedObjects();
	[[nodiscard]] VulkanPipeline createPipeline(PipelineInfo pipelineInfo);
	void addPipeline(VulkanPipeline pipeline);

private:
	//PIPELINE
	void createMainGraphicsPipeline(const char* vertShaderCodePath, const char* fragShaderCodePath);
	void createPipelineLayout();
	[[nodiscard]] vk::ShaderModule createShaderModule(std::vector<char>& shaderCode);

	//RENDER PASS
	[[nodiscard]] vk::Format findDepthFormat();
	void createRenderPass();

	//DESCRIPTORS
	void createDescriptorSetLayout();
	void createDescriptorPool();
	[[nodiscard]] std::vector<vk::DescriptorSet> createDescriptorSets(VulkanScene* scene);
	void createDescriptorObjects();
	void createDefaultTexture();

	//PUSH CONSTANTS
	void createPushConstantRanges();

	//FRAMEBUFFERS
	void createFramebuffers();
	void createFramebufferAttachments();
	
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
	void updatePushConstants(vk::CommandBuffer commandBuffer, Model& model, TexturedMesh& mesh);

	//INPUTS
	void manageInput();

	void renderImGui(vk::CommandBuffer commandBuffer);
};

