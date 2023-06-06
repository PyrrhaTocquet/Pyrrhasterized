#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanContext.h"
#include "Defs.h"
#include "VulkanScene.h"

class VulkanScene;

//Abstract class to implement render passes
class VulkanRenderPass {

protected:
	VulkanContext* m_context = nullptr;
	std::vector<vk::Framebuffer> m_framebuffers;
	vk::RenderPass m_renderPass = VK_NULL_HANDLE;

public :
	vk::Format findDepthFormat() ;
	VulkanRenderPass(VulkanContext* context);
	virtual ~VulkanRenderPass();
	virtual void createRenderPass() = 0;
	virtual void createFramebuffer() = 0;
	virtual void createAttachments() = 0;
	virtual void cleanAttachments() = 0;
	virtual void recreateRenderPass() = 0;
	virtual vk::Extent2D getRenderPassExtent() = 0;
	virtual void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, vk::DescriptorSet descriptorSet, vk::Pipeline pipeline, std::vector<VulkanScene*> scenes, vk::PipelineLayout pipelineLayout) = 0;
	vk::RenderPass getRenderPass();
	vk::Framebuffer getFramebuffer(uint32_t index);
	void cleanFramebuffer();

private:
};