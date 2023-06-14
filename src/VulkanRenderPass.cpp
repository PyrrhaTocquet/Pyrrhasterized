#include "VulkanRenderPass.h"

//returns the best depth format provided by the device used by the VulkanContext
vk::Format VulkanRenderPass::findDepthFormat()
{
    return m_context->findSupportedFormat({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

VulkanRenderPass::VulkanRenderPass(VulkanContext* context)
{
	m_context = context;
}

VulkanRenderPass::~VulkanRenderPass()
{
	m_context->getDevice().destroyDescriptorPool(m_mainDescriptorPool);
	m_context->getDevice().destroyDescriptorSetLayout(m_mainDescriptorSetLayout);
	for (auto& framebuffer : m_framebuffers)
	{
		m_context->getDevice().destroyFramebuffer(framebuffer);
	}
	m_context->getDevice().destroyRenderPass(m_renderPass);
	delete m_mainPipeline;
	m_context->getDevice().destroyPipelineLayout(m_pipelineLayout);

}

vk::RenderPass VulkanRenderPass::getRenderPass()
{
	return m_renderPass;
}

vk::Framebuffer VulkanRenderPass::getFramebuffer(uint32_t index)
{
	return m_framebuffers[index];
}

void VulkanRenderPass::cleanFramebuffer()
{
	for (auto& framebuffer : m_framebuffers) {
		m_context->getDevice().destroyFramebuffer(framebuffer);
	}
}
