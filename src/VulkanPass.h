/*
author: Pyrrha Tocquet
date: 26/10/25
desc: Abstraction of Render/Compute passes that can be handled at VulkanRenderer's level
*/
#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanContext.h"
#include "Defs.h"
#include "VulkanScene.h"

class VulkanScene;

//Abstract class to implement passes
class VulkanPass {

protected:
	VulkanContext					*m_context = nullptr;
	vk::PipelineLayout				m_pipelineLayout = VK_NULL_HANDLE;
	vk::DescriptorPool				m_mainDescriptorPool;
	vk::DescriptorSetLayout			m_mainDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<vk::DescriptorSet>	m_mainDescriptorSet;
	vk::PushConstantRange m_pushConstant;
public :
	VulkanPass();
	VulkanPass(VulkanContext* context);
	virtual ~VulkanPass();
	vk::Format findDepthFormat();
	virtual void recreatePass() = 0;
	virtual void updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes) = 0;
	virtual void executePass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) = 0;
	virtual void updateDescriptorSets() = 0;
	virtual void createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo>) = 0;
};
