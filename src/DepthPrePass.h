/*
author: Pyrrha Tocquet
date: 19/12/23
desc: Depth pre-pass for forward+
*/

#pragma once
#include "VulkanRenderPass.h"

class DepthPrePass : public VulkanRenderPass 
{

private:
	// created
	VulkanImage* m_depthAttachment = nullptr;

	std::vector<VulkanBuffer> m_viewUniformBuffers;
	
	// borrowed
	Camera* m_camera;


public:
	DepthPrePass(VulkanContext* context, Camera* camera);
	virtual ~DepthPrePass();
	virtual void createRenderPass();
	virtual void createFramebuffer();
	virtual void createAttachments();
	virtual void cleanAttachments();
	virtual void recreateRenderPass();
	virtual void createDescriptorPool();
	virtual void createDescriptorSetLayout();
	virtual void createDescriptorSets(VulkanScene* scene);
	virtual void createPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout);
	virtual void createDefaultPipeline();
	virtual void createPipelineRessources();
	virtual void createPushConstantsRanges();
	virtual void updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes);
	virtual vk::Extent2D getRenderPassExtent();
	virtual void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes);
	virtual void updateDescriptorSets() {};
};