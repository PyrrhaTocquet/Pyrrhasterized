#include "VulkanComputePass.h"

// ---------------------------------------------------------------------------------

VulkanComputePass::VulkanComputePass(VulkanContext *context) : VulkanPass(context)
{
}

// ---------------------------------------------------------------------------------

VulkanComputePass::~VulkanComputePass()
{
	if (m_mainPipeline != nullptr)
		delete m_mainPipeline;
}

// ---------------------------------------------------------------------------------
