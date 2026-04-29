#include "VulkanRenderPass.h"

VulkanRenderPass::VulkanRenderPass(VulkanContext *context) : VulkanPass(context)
{
}

// ----------------------------------------------------------------------------------

VulkanRenderPass::~VulkanRenderPass()
{
	for (auto& framebuffer : m_framebuffers)
	{
		m_context->getDevice().destroyFramebuffer(framebuffer);
	}
	m_context->getDevice().destroyRenderPass(m_renderPass);
	delete m_mainPipeline;
}

// ----------------------------------------------------------------------------------

vk::RenderPass VulkanRenderPass::getRenderPass()
{
	return m_renderPass;
}

// ----------------------------------------------------------------------------------

vk::Framebuffer VulkanRenderPass::getFramebuffer(uint32_t index)
{
	return m_framebuffers[index];
}

// ----------------------------------------------------------------------------------

void VulkanRenderPass::cleanFramebuffer()
{
	for (auto& framebuffer : m_framebuffers) {
		m_context->getDevice().destroyFramebuffer(framebuffer);
	}
}
