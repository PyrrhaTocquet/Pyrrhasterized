#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanContext.h"

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
	virtual void createRenderPass() {};
	virtual void createFramebuffer() {};
	virtual void createAttachments() {};
	virtual void cleanAttachments() {};
	virtual void recreateRenderPass() {};
	virtual vk::Extent2D getRenderPassExtent() = 0;
	virtual void drawRenderPass() {};
	vk::RenderPass getRenderPass();
	vk::Framebuffer getFramebuffer(uint32_t index);

private:
};