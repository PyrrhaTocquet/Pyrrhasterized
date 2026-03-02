#include "DepthPrePass.h"

DepthPrePass::DepthPrePass(VulkanContext* context, vk::DescriptorSetLayout geometryDescriptorSetLayout)
:	DepthOnlyPass(context)
{
	createPass();

	createPushConstantsRanges();
    createDepthOnlyDescriptorSetLayout();
    createDepthOnlyDescriptorPool();

    createDepthOnlyPipelineLayout(geometryDescriptorSetLayout);
    createDefaultPipeline();

	createAttachments();
	createDepthOnlyFramebuffer();
}

DepthPrePass::~DepthPrePass()
{
	vk::Device device = m_context->getDevice();
	device.destroyDescriptorPool(m_materialDescriptorPool);
	device.destroyDescriptorSetLayout(m_materialDescriptorSetLayout);
}


void DepthPrePass::createPass()
{
	vk::AttachmentDescription depthDescription
	{
		.format = findDepthFormat(),
		.samples = vk::SampleCountFlagBits::e1,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::eAttachmentOptimal,
	};

	vk::AttachmentReference depthAttachmentRef
	{
		.attachment = 0,
		.layout = vk::ImageLayout::eAttachmentOptimal,
	};

	vk::SubpassDescription subpass
	{
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 0,
		.pDepthStencilAttachment = &depthAttachmentRef,
	};

	vk::SubpassDependency dependencyDepthAttachment
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests |
			vk::PipelineStageFlagBits::eLateFragmentTests,
		.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests |
		vk::PipelineStageFlagBits::eLateFragmentTests,
		.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
		.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
		vk::AccessFlagBits::eDepthStencilAttachmentWrite,
	};

	vk::RenderPassCreateInfo renderPassInfo
	{
		.attachmentCount = 1,
		.pAttachments = &depthDescription,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependencyDepthAttachment,
	};

	if (m_context->getDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPass) != vk::Result::eSuccess)
		throw std::runtime_error("failed to create depth pre-pass render pass");
}

void DepthPrePass::createDepthOnlyFramebuffer()
{
	m_framebuffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::Extent2D extent = getRenderPassExtent();

		vk::FramebufferCreateInfo framebufferInfo
		{
			.renderPass = m_renderPass,
			.attachmentCount = 1,
			.pAttachments = &m_depthAttachment->m_imageView,
			.width = extent.width,
			.height = extent.height,
			.layers = 1,
		};

		try 
		{
			m_framebuffers[i] = m_context->getDevice().createFramebuffer(framebufferInfo, nullptr);
		}
		catch (vk::SystemError)
		{
			throw std::runtime_error("failed to create framebuffer !");
		}
	}
}

void DepthPrePass::createAttachments()
{
	const vk::Extent2D extent = m_context->getSwapchainExtent();

	const VulkanImageParams imageParams
	{
		.width = extent.width,
		.height = extent.height,
		.mipLevels = 1,
		.numSamples = ENABLE_MSAA ? m_context->getMaxUsableSampleCount() : vk::SampleCountFlagBits::e1,
		.format = findDepthFormat(),
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
	};

	const VulkanImageViewParams imageViewParams	{ .aspectFlags = vk::ImageAspectFlagBits::eDepth };

	m_depthAttachment = new VulkanImage(m_context, imageParams, imageViewParams);
}

void DepthPrePass::recreatePass()
{
	m_context->getDevice().destroyPipeline(m_mainPipeline->getPipeline());
	m_mainPipeline->recreatePipeline(getRenderPassExtent());
	cleanDepthOnlyAttachments();
	cleanFramebuffer();
	createAttachments();
	createDepthOnlyFramebuffer();
}

void DepthPrePass::createDefaultPipeline()
{
	RenderPipelineInfo pipelineInfo{
       .taskShaderPath = "shaders/amplificationPBR.spv",
       .meshShaderPath = "shaders/meshPBR.spv",
       .fragShaderPath = "shaders/fragDepthOnly.spv",
    };

	m_mainPipeline = new VulkanRenderPipeline(m_context, pipelineInfo, m_pipelineLayout, m_renderPass, getRenderPassExtent());
}

void DepthPrePass::createPushConstantsRanges()
{
	m_pushConstant = vk::PushConstantRange
	{
        .stageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = 128,
    };
}


vk::Extent2D DepthPrePass::getRenderPassExtent()
{
	return m_context->getSwapchainExtent();
}

void DepthPrePass::executePass(vk::CommandBuffer commandBuffer, uint32_t, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes)
{
	vk::RenderPassBeginInfo renderPassInfo{
       .renderPass = m_renderPass,
       .framebuffer = m_framebuffers[m_currentFrame],
       .renderArea = {
           .offset = {0, 0},
           .extent = getRenderPassExtent(),
       },
       .clearValueCount = static_cast<uint32_t>(SHADOW_DEPTH_CLEAR_VALUES.size()),
       .pClearValues = SHADOW_DEPTH_CLEAR_VALUES.data(),
    };
    ModelPushConstant pushConstant; // TODO Remove useless push constant
	pushConstant.m_shellCount = 1;
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_mainPipeline->getPipeline()); //Only one main draw pipeline per frame in this renderer
    //Draws each scene
    for (auto& scene : scenes)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, { scene->getGeometryDescriptorSet() , m_mainDescriptorSet[m_currentFrame], m_materialDescriptorSet[m_currentFrame] }, nullptr);
		scene->draw(commandBuffer, m_currentFrame, m_pipelineLayout, pushConstant);
    }
    
    commandBuffer.endRenderPass();
}

vk::DescriptorBufferInfo DepthPrePass::getUboInfo(VulkanScene* scene, const uint32_t frame)
{
	return vk::DescriptorBufferInfo 
	{
		.buffer = scene->getGeneralUniformBuffer(frame).m_Buffer,
		.offset = 0,
		.range = sizeof(GeneralUniformBufferObject)
	};
}
