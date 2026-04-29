/*
author: Pyrrha Tocquet
date: 26/10/25
desc: Abstraction of Compute passes
*/
#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include "VulkanPass.h"
#include "VulkanPipeline.h"

//Abstract class to implement render passes
class VulkanComputePass : public VulkanPass {

protected:
	VulkanComputePipeline			*m_mainPipeline = nullptr;
public :
	VulkanComputePass() = default;
	VulkanComputePass(VulkanContext* context);
	VulkanComputePass(const VulkanComputePass&) = delete;
	VulkanComputePass operator=(const VulkanComputePass&) = delete;
	VulkanComputePass(VulkanComputePass&&) = delete;
	VulkanComputePass operator=(VulkanComputePass&&) = delete;

	virtual ~VulkanComputePass();
	virtual void recreatePass() = 0;
	virtual void executePass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) = 0;
	virtual void updateDescriptorSets() {};
};