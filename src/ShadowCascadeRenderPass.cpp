#include "ShadowCascadeRenderPass.h"

ShadowCascadeRenderPass::ShadowCascadeRenderPass(VulkanContext* context, Camera* camera)
{
    m_context = context;
    //m_camera = camera;
    std::vector<vk::ImageView> swapchainImageViews = m_context->getSwapchainImageViews();
    //One per cascade times two per frames frames in flight
    m_framebuffers.resize(m_context->getSwapchainImagesCount() * SHADOW_CASCADE_COUNT);
    m_shadowDepthLayerViews.resize(m_context->getSwapchainImagesCount() * SHADOW_CASCADE_COUNT);

}

ShadowCascadeRenderPass::~ShadowCascadeRenderPass() {
    cleanAttachments();
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
       .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled,
       .useDedicatedMemory = true,
       .layers = SHADOW_CASCADE_COUNT
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
   
}


void ShadowCascadeRenderPass::drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t currentFrame, vk::DescriptorSet descriptorSet, vk::Pipeline pipeline, std::vector<VulkanScene*> scenes, vk::PipelineLayout pipelineLayout)
{
    //RENOMMER currentFrame
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

        renderPassInfo.framebuffer = m_framebuffers[MAX_FRAMES_IN_FLIGHT * currentFrame + i],
     

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        VkDeviceSize offset = 0;
        //Draws each scene
        for (auto& scene : scenes)
        {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, scene->m_descriptorSets[currentFrame], nullptr);
           
            scene->draw(commandBuffer, currentFrame, pipelineLayout);
        }

        commandBuffer.endRenderPass();
    }
   

}

void ShadowCascadeRenderPass::createFramebuffer()
{
    vk::Extent2D extent = getRenderPassExtent();
    vk::Format depthFormat = findDepthFormat();
    //One framebuffer by cascade and two framebuffer per frame in flight
    for (size_t i = 0; i < m_framebuffers.size(); i++) {

        vk::ImageViewCreateInfo viewInfo{
            .image = m_shadowDepthAttachment->m_image,
            .viewType = vk::ImageViewType::e2D,
            .format = findDepthFormat(),
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eDepth,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = i % 4, //framebuffer indexing [currentFrame * MAX_FRAMES_IN_FLIGHT + cascadeId]
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
