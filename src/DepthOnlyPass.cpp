#include "DepthOnlyPass.h"

DepthOnlyPass::DepthOnlyPass(VulkanContext* context)
	: VulkanRenderPass(context)
{
	m_framebuffers.resize(1);
}

DepthOnlyPass::~DepthOnlyPass()
{
	cleanDepthOnlyAttachments();
}

void DepthOnlyPass::cleanDepthOnlyAttachments()
{
	delete m_depthAttachment;

	vk::Device device = m_context->getDevice();

	device.destroyDescriptorPool(m_materialDescriptorPool);
	device.destroyDescriptorSetLayout(m_materialDescriptorSetLayout);
}

void DepthOnlyPass::createDepthOnlyDescriptorPool()
{
	vk::Device device = m_context->getDevice();

	// General UBO and Textures
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

		try 
		{
			m_materialDescriptorPool = device.createDescriptorPool(materialPoolInfo);
		}
		catch (vk::SystemError err)
		{
			throw std::runtime_error("could not create descriptor pool");
		}
	}
}

void DepthOnlyPass::createDepthOnlyDescriptorSetLayout()
{
	vk::Device device = m_context->getDevice();

	vk::DescriptorSetLayoutBinding uboLayoutBinding
	{
		.binding = 0,
		.descriptorType = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
	};

	vk::DescriptorSetLayoutBinding samplerLayoutBinding
	{
		.binding = 1,
		.descriptorType = vk::DescriptorType::eCombinedImageSampler,
		.descriptorCount = MAX_TEXTURE_COUNT,  //Dynamic Indexing
		.stageFlags = vk::ShaderStageFlagBits::eFragment,
	};

	vk::DescriptorSetLayoutBinding bindings[2] = { uboLayoutBinding, samplerLayoutBinding };

	//Descriptor indexing
	vk::DescriptorBindingFlags bindingFlags[2];
	bindingFlags[1] = vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount; //Necessary for Dynamic indexing (VK_EXT_descriptor_indexing)

	vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo
	{
		.bindingCount = 2,
		.pBindingFlags = bindingFlags,
	};

	vk::DescriptorSetLayoutCreateInfo layoutInfo
	{
		.pNext = &bindingFlagsCreateInfo,
		.bindingCount = 2,
		.pBindings = bindings,
	};

	try 
	{
		m_mainDescriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
	}
	catch (vk::SystemError err)
	{
		throw std::runtime_error("could not create descriptor set layout");
	}

	vk::DescriptorSetLayoutBinding materialLayoutBinding
	{
		.binding = 0,
		.descriptorType = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = MAX_MATERIAL_COUNT,
		.stageFlags = vk::ShaderStageFlagBits::eFragment,
	};

	//Set 1: Material data 
	vk::DescriptorBindingFlags bindingFlag;
	bindingFlag = vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount; //Necessary for Dynamic indexing (VK_EXT_descriptor_indexing)

	vk::DescriptorSetLayoutBindingFlagsCreateInfo materialBindingFlagsCreateInfo
	{
		.bindingCount = 1,
		.pBindingFlags = &bindingFlag,
	};

	vk::DescriptorSetLayoutCreateInfo materialLayoutInfo
	{
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

void DepthOnlyPass::createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo> textureImageInfos)
{
	//Creates a vector of descriptorImageInfo from the scene's textures (DUPLICATE, REFACTOR INTO SCENE)
	{
		m_mainDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_mainDescriptorSetLayout);

		/* Dynamic Descriptor Counts */
		uint32_t textureMaxCount = static_cast<uint32_t>(textureImageInfos.size());
		std::vector<uint32_t> textureMaxCounts(MAX_FRAMES_IN_FLIGHT, textureMaxCount);

		vk::DescriptorSetVariableDescriptorCountAllocateInfo setCounts
		{
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

		try 
		{
			m_mainDescriptorSets = m_context->getDevice().allocateDescriptorSets(allocInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("could not allocate descriptor sets");
		}

		//Updating the descriptor sets with the appropriates references
		for (uint32_t currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++) 
		{
			vk::DescriptorBufferInfo uboBufferInfo = getUboInfo(scene, currentFrame);

			std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
			descriptorWrites[0].dstSet = m_mainDescriptorSets[currentFrame];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &uboBufferInfo;

			descriptorWrites[1].dstSet = m_mainDescriptorSets[currentFrame];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].descriptorCount = static_cast<uint32_t>(textureImageInfos.size());
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

		try 
		{
			m_materialDescriptorSet = m_context->getDevice().allocateDescriptorSets(allocInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("could not allocate descriptor sets");
		}
	}

	for (uint32_t currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++) 
	{
		std::vector<vk::DescriptorBufferInfo> materialBufferInfos;

		//Creating and filling the Uniform Buffers for each material
		vk::DeviceSize bufferSize = sizeof(MaterialUBO);

		materialBufferInfos.resize(scene->m_materialCount);
		for (uint32_t i = 0; i < static_cast<uint32_t>(materialBufferInfos.size()); i++) {

			//filling the material buffer infos
			vk::DescriptorBufferInfo bufferInfo{
				.buffer = scene->getMaterialUniformBuffer(currentFrame, i).m_Buffer,
				.offset = 0,
				.range = bufferSize
			};
			materialBufferInfos[i] = bufferInfo;
		}

		vk::WriteDescriptorSet descriptorWriteInfo
		{
			.dstSet = m_materialDescriptorSet[currentFrame],
			.dstBinding = 0,
			.descriptorCount = static_cast<uint32_t>(materialBufferInfos.size()),
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pBufferInfo = materialBufferInfos.data(),
		};

		try {
			m_context->getDevice().updateDescriptorSets(descriptorWriteInfo, nullptr);
		}
		catch (vk::SystemError err)
		{
			throw std::runtime_error("could not create descriptor sets");
		}
	}
}

void DepthOnlyPass::createDepthOnlyPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout)
{
	std::array<vk::DescriptorSetLayout, 3> layouts = { geometryDescriptorSetLayout, m_mainDescriptorSetLayout, m_materialDescriptorSetLayout };

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
		.setLayoutCount = static_cast<uint32_t>(layouts.size()),
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

void DepthOnlyPass::recreatePass()
{
	//No need to recreate attachments, the shadow map is fixed sized
	cleanFramebuffer();
	createDepthOnlyFramebuffer();
}

vk::ImageView DepthOnlyPass::getDepthAttachment()
{
	return m_depthAttachment->m_imageView;
}

