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
#include "DepthPrePass.h"
#include "Material.h"



class ShadowRenderPass;

class MainRenderPass : public VulkanRenderPass {
	//created
	VulkanImage* m_colorAttachment = nullptr;

	vk::Sampler m_shadowMapSampler = VK_NULL_HANDLE;

	vk::DescriptorPool m_materialDescriptorPool;
	vk::DescriptorSetLayout m_materialDescriptorSetLayout;
	std::vector<vk::DescriptorSet> m_materialDescriptorSet;

	Camera* m_camera; 

	//acquired at construction
	ShadowCascadeRenderPass *m_shadowRenderPass = nullptr;
	DepthPrePass *m_depthPrePass = nullptr;

	//IMGUI
	bool m_hideImGui = false;

	int selectedId = 0;
	int shellCount = 128;
	float hairLength = 0.03;
	float gravityFactor = 0.02;
	float hairDensity = 1000.f;
public:
	MainRenderPass(VulkanContext *context, ShadowCascadeRenderPass *shadowRenderPass, DepthPrePass *depthPrePass);
	virtual ~MainRenderPass()override;
	void createRenderPass() override;
	void createFramebuffer() override;
	void createAttachments() override;
	void cleanAttachments() override;
	void recreateRenderPass() override;
	void createDescriptorPool()override;
	void createDescriptorSetLayout()override;
	void createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo> textureImageInfos)override;
	void createPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout)override;
	void createDefaultPipeline()override;
	void createPipelineRessources()override;
	void createPushConstantsRanges()override;
	void updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes)override;
	[[nodiscard]] vk::Extent2D getRenderPassExtent() override;
	void renderImGui(vk::CommandBuffer commandBuffer);
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) override;
private:
	void createShadowMapSampler();
	void createMainDescriptorSet(VulkanScene* scene);
	void createMaterialDescriptorSet(VulkanScene* scene);
	void updateMaterialUniformBuffer(uint32_t currentFrame, std::vector<VulkanScene*> scenes);


};