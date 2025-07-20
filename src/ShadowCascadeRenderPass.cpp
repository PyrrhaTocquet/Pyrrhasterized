#include "ShadowCascadeRenderPass.h"

ShadowCascadeRenderPass::ShadowCascadeRenderPass(VulkanContext* context)
{
	m_context = context;
	std::vector<vk::ImageView> swapchainImageViews = m_context->getSwapchainImageViews();
	//One per cascade times two per frames frames in flight
	m_framebuffers.resize(SHADOW_CASCADE_COUNT);
	m_shadowDepthLayerViews.resize(SHADOW_CASCADE_COUNT);

}

ShadowCascadeRenderPass::~ShadowCascadeRenderPass() {

	for (auto& framebufferImageView : m_shadowDepthLayerViews)
	{
		m_context->getDevice().destroyImageView(framebufferImageView);
	}
}

void ShadowCascadeRenderPass::createAttachments() {
	vk::Extent2D extent = getRenderPassExtent();

	VulkanImageParams imageParams{
		.width = extent.width,
		.height = extent.height,
		.mipLevels = 1,
		.numSamples = vk::SampleCountFlagBits::e1,
		.format = findDepthFormat(),
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
		.useDedicatedMemory = true,
		.layers = SHADOW_CASCADE_COUNT,
	};

	VulkanImageViewParams imageViewParams{
		.aspectFlags = vk::ImageAspectFlagBits::eDepth,
		.type = vk::ImageViewType::e2DArray
	};

	m_depthAttachment = new VulkanImage(m_context, imageParams, imageViewParams);
}


void ShadowCascadeRenderPass::cleanAttachments() {
	for (auto& imageView : m_shadowDepthLayerViews)
	{
		m_context->getDevice().destroyImageView(imageView);
	}
	delete m_depthAttachment;

}

void ShadowCascadeRenderPass::createRenderPass()
{
	vk::AttachmentDescription shadowDepthWriteDescription
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

	vk::AttachmentReference shadowDepthWriteAttachmentRef = {
		.attachment = 0,
		.layout = vk::ImageLayout::eAttachmentOptimal, //layout during render pass
	};

	vk::SubpassDependency inDependency
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL, //implicit first subpass
		.dstSubpass = 0,
		.srcStageMask = vk::PipelineStageFlagBits::eFragmentShader,// output stage so that the swapchain finishes to read the image before we can access it
		.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests,
		.srcAccessMask = vk::AccessFlagBits::eShaderRead, //Waits for it to be written,
		.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
		.dependencyFlags = vk::DependencyFlagBits::eByRegion,
	};

	vk::SubpassDescription subpass
	{
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 0,
		.pDepthStencilAttachment = &shadowDepthWriteAttachmentRef,
	};

	std::array<vk::SubpassDependency, 1> dependencies{ inDependency };
	vk::RenderPassCreateInfo renderPassInfo
	{
		.attachmentCount = 1,
		.pAttachments = &shadowDepthWriteDescription,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data(),
	};

	if (m_context->getDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPass) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create render pass");
	}
}

void ShadowCascadeRenderPass::createDefaultPipeline()
{

	PipelineInfo pipelineInfo
	{
		.taskShaderPath = "shaders/ampCSM.spv",
		.meshShaderPath = "shaders/meshCSM.spv",
		.fragShaderPath = "shaders/fragDepthOnly.spv",
		.cullmode = vk::CullModeFlagBits::eNone,
		.renderPassId = RenderPassesId::ShadowMappingPassId,
		.isMultisampled = false,
		.depthBias = {c_constantDepthBias, c_slopeScaleDepthBias}
	};

	m_mainPipeline = new VulkanPipeline(m_context, pipelineInfo, m_pipelineLayout, m_renderPass, getRenderPassExtent());
}

void ShadowCascadeRenderPass::createPipelineRessources()
{
}

void ShadowCascadeRenderPass::createPushConstantsRanges()
{
	m_pushConstant = vk::PushConstantRange{
		.stageFlags = vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eFragment,
		.offset = 0,
		.size = 128,
	};
}

void ShadowCascadeRenderPass::updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes)
{
	m_sun = scenes[0]->getSun();
}


void ShadowCascadeRenderPass::drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t currentFrame, std::vector<VulkanScene*> scenes)
{
				//Wait for the end of the previous frame operations on the shadow cascade
	vk::ImageMemoryBarrier2 memoryBarrier{
		.srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		.oldLayout = vk::ImageLayout::eUndefined,
		.newLayout = vk::ImageLayout::eAttachmentOptimal,
		.image = m_depthAttachment->m_image,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eDepth,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = SHADOW_CASCADE_COUNT,
		}
	};

	vk::DependencyInfo dependencyInfo = {
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &memoryBarrier,

	};

	commandBuffer.pipelineBarrier2(dependencyInfo);

	vk::RenderPassBeginInfo renderPassInfo{
		.renderPass = m_renderPass,
		.renderArea = {
			.offset = {0, 0},
			.extent = getRenderPassExtent(),
		},
			.clearValueCount = static_cast<uint32_t>(SHADOW_DEPTH_CLEAR_VALUES.size()),
			.pClearValues = SHADOW_DEPTH_CLEAR_VALUES.data(),
	};

	for (uint32_t i = 0; i < SHADOW_CASCADE_COUNT; i++) {

		renderPassInfo.framebuffer = m_framebuffers[i];

		ModelPushConstant pushConstant{
			.cascadeId = i
		};

		commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_mainPipeline->getPipeline());
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, { scenes[0]->getGeometryDescriptorSet(), m_mainDescriptorSet[currentFrame], m_materialDescriptorSet[currentFrame] }, nullptr);
		VkDeviceSize offset = 0;
		//Draws each scene
		for (auto& scene : scenes)
		{
			scene->draw(commandBuffer, currentFrame, m_pipelineLayout, pushConstant);
		}

		commandBuffer.endRenderPass();
	}
	recordShadowCascadeMemoryDependency(commandBuffer);
}

void ShadowCascadeRenderPass::recreateRenderPass()
{
	updateDescriptorSets();
	m_context->getDevice().destroyPipeline(m_mainPipeline->getPipeline());
	m_mainPipeline->recreatePipeline(getRenderPassExtent());
}

//Wait for the end of shadow cascade drawing to use it in the Main render pass
void ShadowCascadeRenderPass::recordShadowCascadeMemoryDependency(vk::CommandBuffer commandBuffer) {

	vk::ImageMemoryBarrier2 memoryBarrier{
		.srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
		.dstAccessMask = vk::AccessFlagBits2::eShaderRead,
		.oldLayout = vk::ImageLayout::eAttachmentOptimal,
		.newLayout = vk::ImageLayout::eReadOnlyOptimal,
		.image = m_depthAttachment->m_image,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eDepth,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = SHADOW_CASCADE_COUNT,
		}
	};

	vk::DependencyInfo dependencyInfo = {
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &memoryBarrier,
	};

	commandBuffer.pipelineBarrier2(dependencyInfo);
}

vk::DescriptorBufferInfo ShadowCascadeRenderPass::getUboInfo(VulkanScene *scene, const uint32_t frame)
{
	return vk::DescriptorBufferInfo 
	{
		.buffer = scene->getShadowCascadeUniformBuffer(frame).m_Buffer,
		.offset = 0,
		.range = sizeof(CascadeUniformObject)
	};
}

vk::Extent2D ShadowCascadeRenderPass::getRenderPassExtent()
{
	return vk::Extent2D
	{
		.width = c_shadowMapSize,
		.height = c_shadowMapSize,
	};
}

void ShadowCascadeRenderPass::createFramebuffer()
{
	vk::Extent2D extent = getRenderPassExtent();
	vk::Format depthFormat = findDepthFormat();
	//One framebuffer by cascade and two framebuffer per frame in flight
	for (uint32_t i = 0; i < m_framebuffers.size(); i++) {

		vk::ImageViewCreateInfo viewInfo{
			.image = m_depthAttachment->m_image,
			.viewType = vk::ImageViewType::e2D,
			.format = depthFormat,
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eDepth,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = i, //framebuffer indexing [currentFrame * SHADOW_CASCADE_COUNT + cascadeId]
				.layerCount = 1,
			}
		};

		m_shadowDepthLayerViews[i] = m_context->getDevice().createImageView(viewInfo);

		vk::FramebufferCreateInfo framebufferInfo
		{
			.renderPass = m_renderPass, //Renderpass that is compatible with the framebuffer
			.attachmentCount = 1,
			.pAttachments = &m_shadowDepthLayerViews[i],
			.width = extent.width,
			.height = extent.height,
			.layers = 1,
		};

		try {
			m_framebuffers[i] = m_context->getDevice().createFramebuffer(framebufferInfo, nullptr);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create framebuffer !");
		}
	}
}
