#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "ShadowCascadeRenderPass.h"



class ShadowRenderPass;

class MainRenderPass : public VulkanRenderPass {
	//created
	VulkanImage* m_colorAttachment = nullptr;
	VulkanImage* m_depthAttachment = nullptr;

	VulkanImage* m_defaultTexture = nullptr;
	VulkanImage* m_defaultNormalMap = nullptr;
	std::vector<vk::Buffer> m_uniformBuffers;
	std::vector<vma::Allocation> m_uniformBuffersAllocations;
	vk::Sampler m_textureSampler = VK_NULL_HANDLE;

	vk::DescriptorPool m_shadowDescriptorPool;
	vk::DescriptorSetLayout m_shadowDescriptorSetLayout;
	std::vector<vk::DescriptorSet> m_shadowDescriptorSet;


	Camera* m_camera;

	//acquired at construction
	ShadowRenderPass* m_shadowRenderPass = nullptr;
public:
	MainRenderPass(VulkanContext* context, Camera* camera, ShadowRenderPass* shadowRenderPass);
	virtual ~MainRenderPass()override;
	void createRenderPass() override;
	void createFramebuffer() override;
	void createAttachments() override;
	void cleanAttachments() override;
	void recreateRenderPass() override;
	void createDescriptorPool()override;
	void createDescriptorSetLayout()override;
	void createDescriptorSet(VulkanScene* scene)override;
	void createPipelineLayout()override;
	void createDefaultPipeline()override;
	void createPipelineRessources()override;
	void createPushConstantsRanges()override;
	void updatePipelineRessources(uint32_t currentFrame)override;
	vk::Extent2D getRenderPassExtent() override;
	void renderImGui(vk::CommandBuffer commandBuffer);
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) override;

private:
	void createDefaultTextures();
	void createUniformBuffer();
	void createTextureSampler();
};