#include "DepthPrePass.h"

DepthPrePass::DepthPrePass(VulkanContext* context)
:	VulkanRenderPass(context)
{
	m_framebuffers.resize(MAX_FRAMES_IN_FLIGHT);
}

DepthPrePass::~DepthPrePass()
{
	vk::Device device = m_context->getDevice();
	device.destroyDescriptorPool(m_materialDescriptorPool);
	device.destroyDescriptorSetLayout(m_materialDescriptorSetLayout);

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

	std::array<vk::DescriptorPoolSize, 2> poolSizes;
	poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_TEXTURE_COUNT; //Dynamic Indexing


	vk::DescriptorPoolCreateInfo poolInfo
	{
		.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	try
	{
		m_mainDescriptorPool = device.createDescriptorPool(poolInfo);
	}
	catch (vk::SystemError)
	{
		throw std::runtime_error("could not create descriptor pool");
	}

	//Materials
	{
		vk::DescriptorPoolSize materialPoolSize;
		materialPoolSize.type = vk::DescriptorType::eUniformBuffer;
		materialPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_MATERIAL_COUNT;

		vk::DescriptorPoolCreateInfo materialPoolInfo{
			.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.poolSizeCount = 1,
			.pPoolSizes = &materialPoolSize,
		};

		try {
			m_materialDescriptorPool = device.createDescriptorPool(materialPoolInfo);
		}
		catch (vk::SystemError err)
		{
			throw std::runtime_error("could not create descriptor pool");
		}
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

	vk::DescriptorSetLayoutBinding samplerLayoutBinding{
		.binding = 1,
		.descriptorType = vk::DescriptorType::eCombinedImageSampler,
		.descriptorCount = MAX_TEXTURE_COUNT,  //Dynamic Indexing
		.stageFlags = vk::ShaderStageFlagBits::eFragment,
	};

	vk::DescriptorSetLayoutBinding bindings[2] = { uboLayoutBinding, samplerLayoutBinding };

	//Descriptor indexing
	vk::DescriptorBindingFlags bindingFlags[2];
	bindingFlags[1] = vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount; //Necessary for Dynamic indexing (VK_EXT_descriptor_indexing)

	vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{
		.bindingCount = 2,
		.pBindingFlags = bindingFlags,
	};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .pNext = &bindingFlagsCreateInfo,
		.bindingCount = 2,
        .pBindings = bindings,
    };

    try {
        m_mainDescriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor set layout");
    }

	vk::DescriptorSetLayoutBinding materialLayoutBinding{
	   .binding = 0,
	   .descriptorType = vk::DescriptorType::eUniformBuffer,
	   .descriptorCount = MAX_MATERIAL_COUNT,
	   .stageFlags = vk::ShaderStageFlagBits::eFragment,
	};

	//Set 1: Material data 
	vk::DescriptorBindingFlags bindingFlag;
	bindingFlag = vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount; //Necessary for Dynamic indexing (VK_EXT_descriptor_indexing)

	vk::DescriptorSetLayoutBindingFlagsCreateInfo materialBindingFlagsCreateInfo{
		.bindingCount = 1,
		.pBindingFlags = &bindingFlag,
	};

	vk::DescriptorSetLayoutCreateInfo materialLayoutInfo{
		.pNext = &materialBindingFlagsCreateInfo,
		.bindingCount = 1,
		.pBindings = &materialLayoutBinding,
	};

	try {
		m_materialDescriptorSetLayout = device.createDescriptorSetLayout(materialLayoutInfo);
	}
	catch (vk::SystemError err)
	{
		throw std::runtime_error("could not create descriptor set layout");
	}
}

void DepthPrePass::createDescriptorSets(VulkanScene* scene)
{
	//Creates a vector of descriptorImageInfo from the scene's textures (DUPLICATE, REFACTOR INTO SCENE)
	{
		std::vector<vk::DescriptorImageInfo> textureImageInfos = scene->generateTextureImageInfo();

		m_mainDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_mainDescriptorSetLayout);

		/* Dynamic Descriptor Counts */
		uint32_t textureMaxCount = textureImageInfos.size();
		std::vector<uint32_t> textureMaxCounts(MAX_FRAMES_IN_FLIGHT, textureMaxCount);
		//std::vector<uint32_t> textureMaxCounts(MAX_FRAMES_IN_FLIGHT, materialMaxCount);
		vk::DescriptorSetVariableDescriptorCountAllocateInfo setCounts{
			.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.pDescriptorCounts = textureMaxCounts.data(),
		};

		/*Descriptor Sets Allocation*/
		vk::DescriptorSetAllocateInfo allocInfo = {
		 .pNext = &setCounts,
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
				.buffer = scene->getGeneralUniformBuffer(currentFrame).m_Buffer,
				.offset = 0,
				.range = sizeof(GeneralUniformBufferObject)
			};

			std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
			descriptorWrites[0].dstSet = m_mainDescriptorSet[currentFrame];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &generalUboBufferInfo;

			descriptorWrites[1].dstSet = m_mainDescriptorSet[currentFrame];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].descriptorCount = textureImageInfos.size();
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
			descriptorWrites[1].pImageInfo = textureImageInfos.data();

			try {
				m_context->getDevice().updateDescriptorSets(descriptorWrites, nullptr);
			}
			catch (vk::SystemError err)
			{
				throw std::runtime_error("could not create descriptor sets");
			}
		}
	}
	{
		m_materialDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
		std::vector<vk::DescriptorSetLayout> materialLayouts(MAX_FRAMES_IN_FLIGHT, m_materialDescriptorSetLayout);

		/* Dynamic Descriptor Counts */
		uint32_t materialMaxCount = MAX_MATERIAL_COUNT;
		std::vector<uint32_t> materialMaxCounts(MAX_FRAMES_IN_FLIGHT, materialMaxCount);
		//std::vector<uint32_t> textureMaxCounts(MAX_FRAMES_IN_FLIGHT, materialMaxCount);
		vk::DescriptorSetVariableDescriptorCountAllocateInfo setCounts{
			.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.pDescriptorCounts = materialMaxCounts.data(),
		};

		vk::DescriptorSetAllocateInfo allocInfo = {
			 .pNext = &setCounts,
			 .descriptorPool = m_materialDescriptorPool,
			 .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			 .pSetLayouts = materialLayouts.data()
		};

		try {
			m_materialDescriptorSet = m_context->getDevice().allocateDescriptorSets(allocInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("could not allocate descriptor sets");
		}
	}
}

void DepthPrePass::createPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout)
{
	 std::array<vk::DescriptorSetLayout, 3> layouts = { geometryDescriptorSetLayout, m_mainDescriptorSetLayout, m_materialDescriptorSetLayout };

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

void DepthPrePass::drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes)
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
	pushConstant.shellCount = 1;
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
