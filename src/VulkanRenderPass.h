#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include "VulkanPass.h"
#include "VulkanPipeline.h"

//Abstract class to implement render passes
class VulkanRenderPass : public VulkanPass {

protected:
	std::vector<vk::Framebuffer>	m_framebuffers;
	vk::RenderPass					m_renderPass = VK_NULL_HANDLE;
	
	VulkanRenderPipeline*			m_mainPipeline = nullptr;
	vk::PushConstantRange			m_pushConstant;
public :
	VulkanRenderPass(VulkanContext* context);
	VulkanRenderPass() = default;
	virtual ~VulkanRenderPass();
	VulkanRenderPass(const VulkanRenderPass&) = delete;
	VulkanRenderPass operator=(const VulkanRenderPass&) = delete;
	VulkanRenderPass(VulkanRenderPass&&) = delete;
	VulkanRenderPass operator=(VulkanRenderPass&&) = delete;


	virtual void recreatePass() = 0;
	virtual void updatePipelineRessources(uint32_t, std::vector<VulkanScene*>) {};
	virtual vk::Extent2D getRenderPassExtent() = 0;
	virtual void executePass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) = 0;
	virtual void updateDescriptorSets() {};
	[[nodiscard]]vk::RenderPass getRenderPass();
	[[nodiscard]]vk::Framebuffer getFramebuffer(uint32_t index);
	virtual void createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo>) = 0;

protected:
	void cleanFramebuffer();
};