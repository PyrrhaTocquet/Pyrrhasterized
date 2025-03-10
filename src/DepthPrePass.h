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


	vk::DescriptorPool m_materialDescriptorPool;
	vk::DescriptorSetLayout m_materialDescriptorSetLayout;
	std::vector<vk::DescriptorSet> m_materialDescriptorSet;

	std::array<std::vector<vk::Buffer>, MAX_FRAMES_IN_FLIGHT> m_materialUniformBuffers;
	std::array<std::vector<vma::Allocation>, MAX_FRAMES_IN_FLIGHT> m_materialUniformBufferAllocations;
	


public:
	DepthPrePass(VulkanContext* context);
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
	virtual void createPushConstantsRanges();

	virtual vk::Extent2D getRenderPassExtent();
	virtual void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes);
	virtual void updateDescriptorSets() {};

	const VulkanImage* getDepthAttachment() { assert(m_depthAttachment != nullptr); return m_depthAttachment; };
};