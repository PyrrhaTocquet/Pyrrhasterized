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

    m_shadowDepthAttachment = new VulkanImage(m_context, imageParams, imageViewParams);
}


void ShadowCascadeRenderPass::cleanAttachments() {
    for (auto& imageView : m_shadowDepthLayerViews)
    {
        m_context->getDevice().destroyImageView(imageView);
    }
    delete m_shadowDepthAttachment;
   
}

void ShadowCascadeRenderPass::createDescriptorPool()
{
    vk::Device device = m_context->getDevice();
    //MVP UBO + TEXTURES
    {
        vk::DescriptorPoolSize poolSize
        {
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        };

        vk::DescriptorPoolCreateInfo poolInfo{
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = static_cast<uint32_t>(1),
            .pPoolSizes = &poolSize,
        };

        try {
            m_mainDescriptorPool = device.createDescriptorPool(poolInfo);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor pool");
        }
    }
}

void ShadowCascadeRenderPass::createDescriptorSetLayout()
{
    vk::Device device = m_context->getDevice();

    //Set 0: Transforms Uniform Buffer, Texture sampler
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
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

void ShadowCascadeRenderPass::createDescriptorSets(VulkanScene* scene)
{
    m_mainDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_mainDescriptorSetLayout);

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
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = scene->getShadowCascadeUniformBuffer(i).m_Buffer,
            .offset = 0,
            .range = sizeof(CascadeUniformObject)
        };

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.dstSet = m_mainDescriptorSet[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        try {
            m_context->getDevice().updateDescriptorSets(descriptorWrite, nullptr);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor sets");
        }
    }

}

void ShadowCascadeRenderPass::createPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout)
{

    std::array<vk::DescriptorSetLayout, 2> layouts {geometryDescriptorSetLayout, m_mainDescriptorSetLayout};
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
       .setLayoutCount = 2,
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

void ShadowCascadeRenderPass::createDefaultPipeline()
{

    PipelineInfo pipelineInfo{
        .taskShaderPath = "shaders/taskShadow.spv",
        .meshShaderPath = "shaders/meshCSM.spv",
        .fragShaderPath = "shaders/fragmentCSM.spv",
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
        .stageFlags = vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eTaskEXT| vk::ShaderStageFlagBits::eFragment,
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
    .image = m_shadowDepthAttachment->m_image,
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
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, { scenes[0]->getGeometryDescriptorSet(), m_mainDescriptorSet[currentFrame]}, nullptr);
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
        .image = m_shadowDepthAttachment->m_image,
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

void ShadowCascadeRenderPass::createFramebuffer()
{
    vk::Extent2D extent = getRenderPassExtent();
    vk::Format depthFormat = findDepthFormat();
    //One framebuffer by cascade and two framebuffer per frame in flight
    for (uint32_t i = 0; i < m_framebuffers.size(); i++) {

        vk::ImageViewCreateInfo viewInfo{
            .image = m_shadowDepthAttachment->m_image,
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

        vk::FramebufferCreateInfo framebufferInfo{
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
