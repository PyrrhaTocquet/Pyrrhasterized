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
class VulkanComputePass : VulkanPass {

protected:
	
	vk::PipelineLayout				m_pipelineLayout = VK_NULL_HANDLE;
	VulkanPipeline*					m_mainPipeline = nullptr;

	vk::DescriptorPool				m_mainDescriptorPool, m_materialDescriptorPool = VK_NULL_HANDLE;
	vk::DescriptorSetLayout			m_mainDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<vk::DescriptorSet>	m_mainDescriptorSet;

	vk::PushConstantRange m_pushConstant;
public :
	vk::Format findDepthFormat() ;
	VulkanComputePass(VulkanContext* context);
	VulkanComputePass() {};
	virtual ~VulkanComputePass();
	virtual void createPass() = 0;
	virtual void recreatePass() = 0;
	virtual void createDescriptorPool() = 0;
	virtual void createDescriptorSetLayout() = 0;
	virtual void createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo>) = 0;
	virtual void createPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout) = 0;
	virtual void createDefaultPipeline() = 0;
	virtual void createPipelineRessources() {};
	virtual void createPushConstantsRanges() = 0;
	virtual void updatePipelineRessources(uint32_t, std::vector<VulkanScene*>) {};
	virtual vk::Extent2D getRenderPassExtent() = 0;
	virtual void executePass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) = 0;
	virtual void updateDescriptorSets() {};

private:
};