#include "DepthPrePass.h"

DepthPrePass::DepthPrePass(VulkanContext* context, Camera* camera)
:	VulkanRenderPass(context)
{
	m_framebuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_camera = camera;
}

DepthPrePass::~DepthPrePass()
{
	vk::Device device = m_context->getDevice();

	cleanAttachments();

}


void DepthPrePass::createRenderPass()
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
		.finalLayout = vk::ImageLayout::eDepthAttachmentOptimal,
	};

	vk::AttachmentReference depthAttachmentRef
	{
		.attachment = 0,
		.layout = vk::ImageLayout::eDepthAttachmentOptimal,
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

void DepthPrePass::cleanAttachments()
{
	delete m_depthAttachment;
}

void DepthPrePass::createFramebuffer()
{
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

void DepthPrePass::recreateRenderPass()
{
	m_context->getDevice().destroyPipeline(m_mainPipeline->getPipeline());
	m_mainPipeline->recreatePipeline(getRenderPassExtent());
	cleanAttachments();
	cleanFramebuffer();
	createAttachments();
	createFramebuffer();
}

void DepthPrePass::createDescriptorPool()
{
	vk::Device device = m_context->getDevice();

	vk::DescriptorPoolSize poolSize
	{
		.type = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
	};

	vk::DescriptorPoolCreateInfo poolInfo
	{
		.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
		.poolSizeCount = 1u,
		.pPoolSizes = &poolSize,
	};

	try
	{
		m_mainDescriptorPool = device.createDescriptorPool(poolInfo);
	}
	catch (vk::SystemError)
	{
		throw std::runtime_error("could not create descriptor pool");
	}
}

void DepthPrePass::createDescriptorSetLayout()
{
	 vk::Device device = m_context->getDevice();

    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding,
    };

    try {
        m_mainDescriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor set layout");
    }
}

void DepthPrePass::createDescriptorSets(VulkanScene* scene)
{
	//Creates a vector of descriptorImageInfo from the scene's textures
    m_mainDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_mainDescriptorSetLayout);

    /*Descriptor Sets Allocation*/
    vk::DescriptorSetAllocateInfo allocInfo = {
     .descriptorPool = m_mainDescriptorPool,
     .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
     .pSetLayouts = layouts.data()
    };

    try {
        m_mainDescriptorSet = m_context->getDevice().allocateDescriptorSets(allocInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("could not allocate descriptor sets");
    }

    //Updating the descriptor sets with the appropriates references
    for (uint32_t currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++) {
        vk::DescriptorBufferInfo generalUboBufferInfo
		{
			.buffer = m_viewUniformBuffers[currentFrame].m_Buffer,
			.offset = 0,
			.range = sizeof(GeneralUniformBufferObject)
        };     

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.dstSet = m_mainDescriptorSet[currentFrame];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &generalUboBufferInfo;

        try {
            m_context->getDevice().updateDescriptorSets(descriptorWrite, nullptr);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor sets");
        }
    }
}

void DepthPrePass::createPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout)
{
	 std::array<vk::DescriptorSetLayout, 2> layouts = { geometryDescriptorSetLayout, m_mainDescriptorSetLayout };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
       .setLayoutCount = layouts.size(),
       .pSetLayouts = layouts.data(),
       .pushConstantRangeCount = 1,
       .pPushConstantRanges = &m_pushConstant,
    };

    try {
        m_pipelineLayout = m_context->getDevice().createPipelineLayout(pipelineLayoutInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void DepthPrePass::createDefaultPipeline()
{
	PipelineInfo pipelineInfo{
       .taskShaderPath = "shaders/taskShell.spv",
       .meshShaderPath = "shaders/meshPBR.spv",
       .fragShaderPath = "shaders/fragmentDepthPrePass.spv",
    };

	m_mainPipeline = new VulkanPipeline(m_context, pipelineInfo, m_pipelineLayout, m_renderPass, getRenderPassExtent());
}

void DepthPrePass::createPipelineRessources()
{
	//General UBO
    vk::DeviceSize bufferSize = sizeof(GeneralUniformBufferObject);
    m_viewUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::tie(m_viewUniformBuffers[i].m_Buffer, m_viewUniformBuffers[i].m_Allocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu, "General Uniform Buffer");
    }
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

void DepthPrePass::updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes)
{
	vk::Extent2D extent = getRenderPassExtent();

    //Model View Proj
    GeneralUniformBufferObject ubo{};
    ubo.view = m_camera->getViewMatrix();
    ubo.proj = m_camera->getProjMatrix(m_context);
    ubo.cameraPos = m_camera->getCameraPos();
    ubo.time = m_context->getTime().elapsedSinceStart;
	// TODO Proper struct for this pass
}

vk::Extent2D DepthPrePass::getRenderPassExtent()
{
	return vk::Extent2D();
}

void DepthPrePass::drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes)
{
	vk::RenderPassBeginInfo renderPassInfo{
       .renderPass = m_renderPass,
       .framebuffer = m_framebuffers[m_currentFrame],
       .renderArea = {
           .offset = {0, 0},
           .extent = getRenderPassExtent(),
       },
       .clearValueCount = static_cast<uint32_t>(MAIN_CLEAR_VALUES.size()),
       .pClearValues = MAIN_CLEAR_VALUES.data(),
    };
    ModelPushConstant pushConstant; // TODO Remove useless push constant
	pushConstant.shellCount = 1;
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_mainPipeline->getPipeline()); //Only one main draw pipeline per frame in this renderer
    //Draws each scene
    for (auto& scene : scenes)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, { scene->getGeometryDescriptorSet() , m_mainDescriptorSet[m_currentFrame] }, nullptr);
        scene->draw(commandBuffer, m_currentFrame, m_pipelineLayout, pushConstant);
    }
    
    commandBuffer.endRenderPass();
}
