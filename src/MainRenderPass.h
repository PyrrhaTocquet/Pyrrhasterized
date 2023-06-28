/*
author: Pyrrha Tocquet
date: 30/05/23
desc: Manages the render pass that draws the final image
*/
#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "ShadowCascadeRenderPass.h"
#include "Material.h"



class ShadowRenderPass;

class MainRenderPass : public VulkanRenderPass {
	//created
	VulkanImage* m_colorAttachment = nullptr;
	VulkanImage* m_depthAttachment = nullptr;

	VulkanImage* m_defaultTexture = nullptr;
	VulkanImage* m_defaultNormalMap = nullptr;
	std::vector<vk::Buffer> m_generalUniformBuffers;
	std::vector<vma::Allocation> m_generalUniformBuffersAllocations;
	std::vector<vk::Buffer> m_lightUniformBuffers;
	std::vector<vma::Allocation> m_lightUniformBuffersAllocations;
	std::vector<vk::Buffer> m_materialUniformBuffer;
	std::vector<vma::Allocation> m_materialUniformBufferAllocations;
	
	vk::Sampler m_shadowMapSampler = VK_NULL_HANDLE;

	vk::DescriptorPool m_shadowDescriptorPool;
	vk::DescriptorSetLayout m_shadowDescriptorSetLayout;
	std::vector<vk::DescriptorSet> m_shadowDescriptorSet;

	Material* material = nullptr;
	float metallicFactorGui = 0.f;
	float roughnessFactorGui = 1.0f;

	Camera* m_camera;

	//acquired at construction
	ShadowCascadeRenderPass* m_shadowRenderPass = nullptr;

	//IMGUI
	bool m_hideImGui = false;

	const std::string c_defaultTexturePath = "assets/defaultTexture.png";
	const std::string c_defaultNormalMapPath = "assets/defaultNormalMap.png";
public:
	MainRenderPass(VulkanContext* context, Camera* camera, ShadowCascadeRenderPass* shadowRenderPass);
	virtual ~MainRenderPass()override;
	void createRenderPass() override;
	void createFramebuffer() override;
	void createAttachments() override;
	void cleanAttachments() override;
	void recreateRenderPass() override;
	void createDescriptorPool()override;
	void createDescriptorSetLayout()override;
	void createDescriptorSets(VulkanScene* scene)override;
	void createPipelineLayout()override;
	void createDefaultPipeline()override;
	void createPipelineRessources()override;
	void createPushConstantsRanges()override;
	void updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes)override;
	[[nodiscard]] vk::Extent2D getRenderPassExtent() override;
	void renderImGui(vk::CommandBuffer commandBuffer);
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) override;

private:
	void createDefaultTextures();
	void createUniformBuffers();
	void createShadowMapSampler();
	std::vector<vk::DescriptorImageInfo> generateTextureImageInfo(VulkanScene* scene);
	void createMainDescriptorSet(VulkanScene* scene);
	void createShadowDescriptorSet(VulkanScene* scene);

	void updateGeneralUniformBuffer(uint32_t currentFrame);
	void updateLightUniformBuffer(uint32_t currentFrame, std::vector<VulkanScene*> scenes);
	void updateMaterialUniformBuffer(uint32_t currentFrame, std::vector<VulkanScene*> scenes);
};