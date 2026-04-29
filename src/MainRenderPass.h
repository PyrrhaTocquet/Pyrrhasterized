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



class DepthOnlyPass;

class MainRenderPass : public VulkanRenderPass {
	//created
	VulkanImage* m_colorAttachment = nullptr;

	vk::Sampler m_shadowMapSampler = VK_NULL_HANDLE;

	vk::DescriptorPool m_materialDescriptorPool;
	vk::DescriptorSetLayout m_materialDescriptorSetLayout;
	std::vector<vk::DescriptorSet> m_materialDescriptorSet;

	Camera* m_camera = nullptr; 

	//acquired at construction
	ShadowCascadeRenderPass *m_shadowRenderPass = nullptr;
	DepthPrePass *m_depthPrePass = nullptr;

	//IMGUI
	bool m_hideImGui = false;

	uint32_t m_selectedShellCountId = 0;
	uint32_t m_shellCount = 128;
	float m_hairLength = 0.03f;
	float m_gravityFactor = 0.02f;
	float m_hairDensity = 1000.f;
public:
	MainRenderPass() = default;
	MainRenderPass(VulkanContext *context, vk::DescriptorSetLayout geometryDescriptorSetLayout, ShadowCascadeRenderPass *shadowRenderPass, DepthPrePass *depthPrePass);
	virtual ~MainRenderPass()override;
	void recreatePass() override;
	[[nodiscard]] vk::Extent2D getRenderPassExtent() override;
	void renderImGui(vk::CommandBuffer commandBuffer);
	void executePass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) override;
private:
	void createPass();
	void createDescriptorPool();
	void createDescriptorSetLayout();
	void createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo> textureImageInfos);
	void createPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout);
	void createDefaultPipeline();
	void createPipelineRessources();
	void createPushConstantsRanges();

	void createFramebuffer();
	void createAttachments();
	void cleanAttachments();

	void createShadowMapSampler();
	void createMainDescriptorSet(VulkanScene* scene);
	void createMaterialDescriptorSet(VulkanScene* scene);
};