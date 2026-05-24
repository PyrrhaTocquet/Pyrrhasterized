#include "LightCullingPass.h"

LightCullingPass::LightCullingPass(VulkanContext *context)
	: VulkanComputePass(context)
{

	m_extent = m_context->getSwapchainExtent();
	createDescriptorSetLayout();
	createDescriptorPool();
	createPipelineLayout();
	createDefaultPipeline();
	createBuffers();
}

// ---------------------------------------------------------------------------------

LightCullingPass::~LightCullingPass()
{
	vk::Device device = m_context->getDevice();
	vma::Allocator *allocator = m_context->getAllocator();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		allocator->destroyBuffer(m_dispatchParamsBuffer[i].m_Buffer, m_dispatchParamsBuffer[i].m_Allocation);
		allocator->destroyBuffer(m_outFrustums[i].m_Buffer, m_outFrustums[i].m_Allocation);
	}
}

// ---------------------------------------------------------------------------------

void LightCullingPass::recreatePass()
{
	 m_context->getDevice().destroyPipeline(m_mainPipeline->getPipeline());
	 m_mainPipeline->recreatePipeline(vk::Extent2D{});
	 m_extent = m_context->getSwapchainExtent();

	updateDispatchParams();
	{
		vk::DeviceSize bufferSize = static_cast<vk::DeviceSize>(m_threadsDispatched[0]) * m_threadsDispatched[1] * sizeof(Frustum);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_context->getAllocator()->destroyBuffer(m_outFrustums[i].m_Buffer, m_outFrustums[i].m_Allocation);
			m_outFrustums[i] = m_context->createBuffer(
				bufferSize,
				vk::BufferUsageFlagBits::eStorageBuffer,
				vma::MemoryUsage::eGpuOnly,
				"Out Frustums Buffer");

			vk::DescriptorBufferInfo outFrustums
			{
				.buffer = m_outFrustums[i].m_Buffer,
				.offset = 0,
				.range = static_cast<vk::DeviceSize>(m_threadsDispatched[0]) * m_threadsDispatched[1] * sizeof(Frustum),
			};

			vk::WriteDescriptorSet writeSet
			{
				.dstSet = m_mainDescriptorSets[i],
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.pBufferInfo = &outFrustums,
			};

			try {
				m_context->getDevice().updateDescriptorSets(writeSet, nullptr);
			}
			catch (vk::SystemError err)
			{
				throw std::runtime_error("could not create descriptor sets");
			}
		}
	}
}

// ---------------------------------------------------------------------------------

void LightCullingPass::executePass(vk::CommandBuffer commandBuffer, uint32_t, uint32_t currentFrame, std::vector<VulkanScene *>)
{
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_mainPipeline->getPipeline());
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipelineLayout, 0, m_mainDescriptorSets[currentFrame], nullptr);

	commandBuffer.dispatch(m_dispatchGroupCount[0], m_dispatchGroupCount[1], 1u);
}

// ---------------------------------------------------------------------------------

void LightCullingPass::createDescriptorSetLayout()
{
	vk::Device device = m_context->getDevice();

	// Set 0: General data, Dispatch Params, out Frustums
	constexpr vk::DescriptorSetLayoutBinding generalBdg
	{
		.binding = 0,
		.descriptorType = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eCompute,
	};

	constexpr vk::DescriptorSetLayoutBinding dispatchBdg
	{
		.binding = 1,
		.descriptorType = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eCompute,
	};

	constexpr vk::DescriptorSetLayoutBinding outFrustumsBdg
	{
		.binding = 2,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eCompute,
	};

	constexpr std::array<vk::DescriptorSetLayoutBinding, 3> bindings({generalBdg, dispatchBdg, outFrustumsBdg});

	const vk::DescriptorSetLayoutCreateInfo layoutInfo
	{
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data(),
	};

	try
	{
		m_mainDescriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
		SET_DEBUG_NAME(m_mainDescriptorSetLayout, VkDescriptorSetLayout, "LightCullingPass main descriptor set layout");
	}
	catch (vk::SystemError err)
	{
		throw std::runtime_error("could not create descriptor set layout");
	}
}

// ---------------------------------------------------------------------------------

void LightCullingPass::createDescriptorPool()
{
	vk::Device device = m_context->getDevice();
	const uint32_t maxFrameInFlight = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	constexpr std::array<vk::DescriptorPoolSize, 3> poolSizes
	{
		vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, maxFrameInFlight},
		vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, maxFrameInFlight},
		vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, maxFrameInFlight},
	};

	const vk::DescriptorPoolCreateInfo poolInfo
	{
		.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
    };

    try {
		m_mainDescriptorPool = device.createDescriptorPool(poolInfo);
		SET_DEBUG_NAME(m_mainDescriptorPool, VkDescriptorPool, "LightCullingPass main descriptor pool");
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor pool");
    }
}

// ---------------------------------------------------------------------------------

void LightCullingPass::createPipelineLayout()
{
	vk::PipelineLayoutCreateInfo info
	{
		.setLayoutCount = 1,
		.pSetLayouts = &m_mainDescriptorSetLayout,
	};

	try {
		m_pipelineLayout = m_context->getDevice().createPipelineLayout(info);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

// ---------------------------------------------------------------------------------

void LightCullingPass::createDefaultPipeline()
{
	ComputePipelineInfo info
	{
		.computePath = "shaders/frustumGrid.spv",
		.computePassId = PassesId::LightCulling
	};

	m_mainPipeline = new VulkanComputePipeline(m_context, info, m_pipelineLayout);
}

// ---------------------------------------------------------------------------------

void LightCullingPass::createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo>)
{
	m_mainDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	const std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{m_mainDescriptorSetLayout, m_mainDescriptorSetLayout};

	vk::DescriptorSetAllocateInfo allocInfo = {
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

	for (uint32_t currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++) 
		{
			vk::DescriptorBufferInfo uboBufferInfo
			{
				.buffer = scene->getGeneralUniformBuffer(currentFrame).m_Buffer,
				.offset = 0,
				.range = sizeof(GeneralUniformBufferObject)
			};

			vk::DescriptorBufferInfo dispatchBufferInfo
			{
				.buffer = m_dispatchParamsBuffer[currentFrame].m_Buffer,
				.offset = 0,
				.range = sizeof(FrustumGridDispatchParams)
			};

			vk::DescriptorBufferInfo outFrustums
			{
				.buffer = m_outFrustums[currentFrame].m_Buffer,
				.offset = 0,
				.range = static_cast<vk::DeviceSize>(m_threadsDispatched[0]) * m_threadsDispatched[1] * sizeof(Frustum),
			};

			std::array<vk::WriteDescriptorSet, 3> descriptorWrites;
			descriptorWrites[0].dstSet = m_mainDescriptorSets[currentFrame];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &uboBufferInfo;

			descriptorWrites[1].dstSet = m_mainDescriptorSets[currentFrame];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = vk::DescriptorType::eUniformBuffer;
			descriptorWrites[1].pBufferInfo = &dispatchBufferInfo;

			descriptorWrites[2].dstSet = m_mainDescriptorSets[currentFrame];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = vk::DescriptorType::eStorageBuffer;
			descriptorWrites[2].pBufferInfo = &outFrustums;

			try {
				m_context->getDevice().updateDescriptorSets(descriptorWrites, nullptr);
			}
			catch (vk::SystemError err)
			{
				throw std::runtime_error("could not create descriptor sets");
			}
		}
}

// ---------------------------------------------------------------------------------

void LightCullingPass::createBuffers()
{
	{
		vk::DeviceSize bufferSize = sizeof(FrustumGridDispatchParams);

		for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_dispatchParamsBuffer[i] = m_context->createBuffer(
				bufferSize,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vma::MemoryUsage::eCpuToGpu,
				"Frustum Tiling Dispatch Params Buffer");
		}
	}
	updateDispatchParams();
	{
		vk::DeviceSize bufferSize = static_cast<vk::DeviceSize>(m_threadsDispatched[0]) * m_threadsDispatched[1] * sizeof(Frustum);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_outFrustums[i] = m_context->createBuffer(
				bufferSize,
				vk::BufferUsageFlagBits::eStorageBuffer,
				vma::MemoryUsage::eGpuOnly,
				"Out Frustums Buffer");
		}
	}
}

// ---------------------------------------------------------------------------------

void LightCullingPass::updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*>)
{
	updateDispatchParams();

	const FrustumGridDispatchParams params
	{
		.threadGroupsCount = { m_dispatchGroupCount[0], m_dispatchGroupCount[1] },
		.executedThreadsCount = { m_threadsDispatched[0], m_threadsDispatched[1] },
	};

	void* data = m_context->getAllocator()->mapMemory(m_dispatchParamsBuffer[currentFrame].m_Allocation);
	memcpy(data, &params, sizeof(FrustumGridDispatchParams));
	m_context->getAllocator()->unmapMemory(m_dispatchParamsBuffer[currentFrame].m_Allocation);
}


// ---------------------------------------------------------------------------------

void LightCullingPass::updateDispatchParams()
{
	m_threadsDispatched[0] = static_cast<uint32_t>(std::ceil(static_cast<uint32_t>(m_extent.width) / 16.f));
	m_threadsDispatched[1] = static_cast<uint32_t>(std::ceil(static_cast<uint32_t>(m_extent.height) / 16.f));

	m_dispatchGroupCount[0] = static_cast<uint32_t>(std::ceil(m_threadsDispatched[0] / 16.f));
	m_dispatchGroupCount[1] = static_cast<uint32_t>(std::ceil(m_threadsDispatched[1] / 16.f));
}