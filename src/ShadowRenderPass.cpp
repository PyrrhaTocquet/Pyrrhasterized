#include "ShadowRenderPass.h"

ShadowRenderPass::ShadowRenderPass(VulkanContext* context) : VulkanRenderPass(context)
{

}

ShadowRenderPass::~ShadowRenderPass()
{
    cleanAttachments();
}

void ShadowRenderPass::createRenderPass()
{
    vk::AttachmentDescription shadowDepthWriteDescription{
     .format = findDepthFormat(),
     .samples = vk::SampleCountFlagBits::e1,
     .loadOp = vk::AttachmentLoadOp::eClear,
     .storeOp = vk::AttachmentStoreOp::eStore,
     .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
     .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
     .initialLayout = vk::ImageLayout::eUndefined,
     .finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal,
    };

    vk::AttachmentReference shadowDepthWriteAttachmentRef = {
        .attachment = 0,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal, //layout during render pass
    };

    vk::SubpassDependency inDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL, //implicit first subpass
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eFragmentShader,// output stage so that the swapchain finishes to read the image before we can access it
        .dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eShaderRead, //Waits for it to be written,
        .dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dependencyFlags = vk::DependencyFlagBits::eByRegion,
    };

    vk::SubpassDependency outDependency{
        .srcSubpass = 0, //implicit first subpass
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eFragmentShader,
        .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite, //Waits for it to be written,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .dependencyFlags = vk::DependencyFlagBits::eByRegion
    };

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 0,
        .pDepthStencilAttachment = &shadowDepthWriteAttachmentRef,
    };

    std::array<vk::SubpassDependency, 2> dependencies{ inDependency, outDependency };
    vk::RenderPassCreateInfo renderPassInfo{
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


void ShadowRenderPass::createAttachments() {
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
    };

    VulkanImageViewParams imageViewParams{
     .aspectFlags = vk::ImageAspectFlagBits::eDepth,
    };

    m_shadowDepthAttachment = new VulkanImage(m_context, imageParams, imageViewParams);
}

void ShadowRenderPass::cleanAttachments()
{
    delete m_shadowDepthAttachment;
}

void ShadowRenderPass::recreateRenderPass()
{
    //No need to recreate attachments, the shadow map is fixed sized
    cleanFramebuffer();
    createFramebuffer();
}

void ShadowRenderPass::drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, vk::DescriptorSet descriptorSet, vk::Pipeline pipeline, std::vector<VulkanScene*> scenes, vk::PipelineLayout pipelineLayout)
{
    vk::RenderPassBeginInfo renderPassInfo{
       .renderPass = m_renderPass,
       .framebuffer = m_framebuffers[swapchainImageIndex],
       .renderArea = {
           .offset = {0, 0},
           .extent = getRenderPassExtent(),
       },
       .clearValueCount = static_cast<uint32_t>(SHADOW_DEPTH_CLEAR_VALUES.size()),
       .pClearValues = SHADOW_DEPTH_CLEAR_VALUES.data(),
    };

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    VkDeviceSize offset = 0;
    //Draws each scene
    for (auto& scene : scenes)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, scene->m_descriptorSets[m_currentFrame], nullptr);
        scene->draw(commandBuffer, m_currentFrame, pipelineLayout);
    }

    commandBuffer.endRenderPass();
}

vk::Extent2D ShadowRenderPass::getRenderPassExtent()
{
    return vk::Extent2D{
            .width = SHADOW_MAP_SIZE,
            .height = SHADOW_MAP_SIZE,
    };
}

vk::ImageView ShadowRenderPass::getShadowAttachment()
{
    return m_shadowDepthAttachment->m_imageView;
}

void ShadowRenderPass::createFramebuffer()
{
    vk::Extent2D extent = getRenderPassExtent();

    for (size_t i = 0; i < m_framebuffers.size(); i++) {

        vk::FramebufferCreateInfo framebufferInfo{
           .renderPass = m_renderPass, //Renderpass that is compatible with the framebuffer
           .attachmentCount = 1,
           .pAttachments = &m_shadowDepthAttachment->m_imageView,
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

