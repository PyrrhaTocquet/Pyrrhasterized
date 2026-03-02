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
	vk::DescriptorPool				m_materialDescriptorPool = VK_NULL_HANDLE;
public :
	VulkanRenderPass(VulkanContext* context);
	VulkanRenderPass();
	virtual ~VulkanRenderPass();
	virtual void createPass() = 0;
	virtual void createFramebuffer() = 0;
	virtual void createAttachments() = 0;
	virtual void cleanAttachments() = 0;
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
	[[nodiscard]]vk::RenderPass getRenderPass();
	[[nodiscard]]vk::Framebuffer getFramebuffer(uint32_t index);
	void cleanFramebuffer();

private:
};