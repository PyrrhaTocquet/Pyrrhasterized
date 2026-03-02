/*
author: Pyrrha Tocquet
date: 07/06/23
desc: Remnants of regular (non cascaded) shadow mapping
07/04/25 became a depth only pass abstract class
TODO Refactor
*/

#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanRenderPass.h"


class DepthOnlyPass : public VulkanRenderPass 
{
protected:
	VulkanImage* m_depthAttachment = nullptr;

	vk::DescriptorPool m_materialDescriptorPool;
	vk::DescriptorSetLayout m_materialDescriptorSetLayout;
	std::vector<vk::DescriptorSet> m_materialDescriptorSet;
public:
	DepthOnlyPass(VulkanContext* context);
	DepthOnlyPass() {};
	virtual ~DepthOnlyPass() override;
	virtual void createPass() = 0;
	virtual void createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo> textureImageInfos)override;
	virtual void recreatePass() = 0;
	virtual void executePass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) = 0;
	virtual vk::Extent2D getRenderPassExtent() = 0;
	[[nodiscard]] vk::ImageView getDepthAttachment();

protected:
	virtual void createDepthOnlyFramebuffer() = 0; // Used for recreatePass
	virtual void createDepthOnlyDescriptorPool(); //TODO
	virtual void createDepthOnlyDescriptorSetLayout();
	virtual void createDepthOnlyPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout);
	virtual void cleanDepthOnlyAttachments();

private:
	virtual vk::DescriptorBufferInfo getUboInfo(VulkanScene *scene, const uint32_t frame) = 0;
};