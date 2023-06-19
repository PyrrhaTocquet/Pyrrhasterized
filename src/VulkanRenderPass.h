#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanContext.h"
#include "Defs.h"
#include "VulkanScene.h"
#include "VulkanPipeline.h"

class VulkanScene;

//Abstract class to implement render passes
class VulkanRenderPass {

protected:
	VulkanContext* m_context = nullptr;
	std::vector<vk::Framebuffer> m_framebuffers;
	vk::RenderPass m_renderPass = VK_NULL_HANDLE;

	vk::PipelineLayout m_pipelineLayout;
	VulkanPipeline* m_mainPipeline;

	vk::DescriptorPool m_mainDescriptorPool;
	vk::DescriptorSetLayout m_mainDescriptorSetLayout;
	std::vector<vk::DescriptorSet> m_mainDescriptorSet;

	vk::PushConstantRange m_pushConstant;
public :
	vk::Format findDepthFormat() ;
	VulkanRenderPass(VulkanContext* context);
	VulkanRenderPass() {};
	virtual ~VulkanRenderPass();
	virtual void createRenderPass() = 0;
	virtual void createFramebuffer() = 0;
	virtual void createAttachments() = 0;
	virtual void cleanAttachments() = 0;
	virtual void recreateRenderPass() = 0;
	virtual void createDescriptorPool() = 0;
	virtual void createDescriptorSetLayout() = 0;
	virtual void createDescriptorSet(VulkanScene* scene) = 0;
	virtual void createPipelineLayout() = 0;
	virtual void createDefaultPipeline() = 0;
	virtual void createPipelineRessources() = 0;
	virtual void createPushConstantsRanges() = 0;
	virtual void updatePipelineRessources(uint32_t currentFrame) = 0;
	virtual vk::Extent2D getRenderPassExtent() = 0;
	virtual void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) = 0;
	virtual void updateDescriptorSets() {};
	[[nodiscard]]vk::RenderPass getRenderPass();
	[[nodiscard]]vk::Framebuffer getFramebuffer(uint32_t index);
	void cleanFramebuffer();

private:
};