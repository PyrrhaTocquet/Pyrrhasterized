#include "VulkanPass.h"

// ----------------------------------------------------------------------------------

VulkanPass::VulkanPass()
{
}

// ----------------------------------------------------------------------------------

VulkanPass::VulkanPass(VulkanContext *context)
{
	m_context = context;
}

VulkanPass::~VulkanPass()
{
	m_context->getDevice().destroyDescriptorPool(m_mainDescriptorPool);
	m_context->getDevice().destroyDescriptorSetLayout(m_mainDescriptorSetLayout);
	m_context->getDevice().destroyPipelineLayout(m_pipelineLayout);
}

// ----------------------------------------------------------------------------------
//returns the best depth format provided by the device used by the VulkanContext
vk::Format VulkanPass::findDepthFormat()
{
    return m_context->findSupportedFormat({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}