/*
author: Pyrrha Tocquet
date: 19/12/23
desc: Depth pre-pass for forward+
*/

#pragma once
#include "DepthOnlyPass.h"

class DepthPrePass : public DepthOnlyPass 
{

private:
	std::array<std::vector<vk::Buffer>, MAX_FRAMES_IN_FLIGHT> m_materialUniformBuffers;
	std::array<std::vector<vma::Allocation>, MAX_FRAMES_IN_FLIGHT> m_materialUniformBufferAllocations;

public:
	DepthPrePass(VulkanContext* context);
	virtual ~DepthPrePass();
	virtual void createRenderPass();
	virtual void createFramebuffer();
	virtual void createAttachments();
	virtual void recreateRenderPass();
	virtual void createDefaultPipeline();
	virtual void createPushConstantsRanges();

	virtual vk::Extent2D getRenderPassExtent();
	virtual void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes);
	virtual void updateDescriptorSets() {};

	const VulkanImage* getDepthAttachment() { assert(m_depthAttachment != nullptr); return m_depthAttachment; };

private:
	vk::DescriptorBufferInfo getUboInfo(VulkanScene *scene, const uint32_t frame);


};
