#include "MainRenderPass.h"

//Always create this render pass after creating the shadow render pass
MainRenderPass::MainRenderPass(VulkanContext* context, ShadowRenderPass* shadowRenderPass) : VulkanRenderPass(context)
{
    m_shadowRenderPass = shadowRenderPass;
}

MainRenderPass::~MainRenderPass()
{
    cleanAttachments();
}

void MainRenderPass::createRenderPass()
{
    vk::SampleCountFlagBits msaaSampleCount = m_context->getMaxUsableSampleCount();

    //MSAA color target
    vk::AttachmentDescription colorDescription{
        .format = m_context->getSwapchainFormat(),
        .samples = ENABLE_MSAA ? msaaSampleCount : vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare, //Best practice with multisampled images is to make the best of lazy allocation with don't care
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = ENABLE_MSAA ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = ENABLE_MSAA ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR,
    };

    //MSAA depth target
    vk::AttachmentDescription depthDescription{
        .format = findDepthFormat(),
        .samples = ENABLE_MSAA ? msaaSampleCount : vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    vk::AttachmentDescription shadowMapReadDescription{
        .format = findDepthFormat(),
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal,
        .finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal, //layout after render pass
    };

    vk::AttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    //Color resolve target
    vk::AttachmentDescription colorDescriptionResolve{
        .format = m_context->getSwapchainFormat(),
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentReference colorAttachmentResolveRef = {
        .attachment = 3,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };


    vk::AttachmentReference shadowMapReadRef = {
        .attachment = 2,
        .layout = vk::ImageLayout::eDepthStencilReadOnlyOptimal
    };

    vk::SubpassDescription subpass = vk::SubpassDescription{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .inputAttachmentCount = 1,
        .pInputAttachments = &shadowMapReadRef,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = ENABLE_MSAA ? &colorAttachmentResolveRef : nullptr,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    std::vector<vk::AttachmentDescription> attachments{ colorDescription, depthDescription, shadowMapReadDescription };
    if (ENABLE_MSAA)attachments.push_back(colorDescriptionResolve);

    vk::RenderPassCreateInfo renderPassInfo{
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
    };

    if (m_context->getDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPass) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create main render pass");
    }
}


void MainRenderPass::createAttachments() {
    vk::Extent2D extent = m_context->getSwapchainExtent();

    VulkanImageParams imageParams{
        .width = extent.width,
        .height = extent.height,
        .mipLevels = 1,
        .numSamples = ENABLE_MSAA ? m_context->getMaxUsableSampleCount() : vk::SampleCountFlagBits::e1,
        .format = m_context->getSwapchainFormat(),
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        .useDedicatedMemory = true,
    };

    VulkanImageViewParams imageViewParams{
        .aspectFlags = vk::ImageAspectFlagBits::eColor,
    };

    if (ENABLE_MSAA)m_colorAttachment = new VulkanImage(m_context, imageParams, imageViewParams);

    imageParams.format = findDepthFormat();
    imageParams.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eDepthStencilAttachment;
    imageViewParams.aspectFlags = vk::ImageAspectFlagBits::eDepth;

    m_depthAttachment = new VulkanImage(m_context, imageParams, imageViewParams);
}

void MainRenderPass::cleanAttachments()
{
    if (ENABLE_MSAA)delete m_colorAttachment;
    delete m_depthAttachment;
}

void MainRenderPass::recreateRenderPass()
{
    cleanAttachments();
    cleanFramebuffer();
    createAttachments();
    createFramebuffer();

}

vk::Extent2D MainRenderPass::getRenderPassExtent()
{
    return m_context->getSwapchainExtent();
}

void MainRenderPass::createFramebuffer() {
    std::vector<vk::ImageView> swapchainImageViews = m_context->getSwapchainImageViews();
    std::vector<vk::ImageView> attachments;
    for (size_t i = 0; i < m_framebuffers.size(); i++) {
        if (ENABLE_MSAA)
        {
            attachments = { //Order corresponds to the attachment references
            m_colorAttachment->m_imageView,
            m_depthAttachment->m_imageView,
            m_shadowRenderPass->getShadowAttachment(),
            swapchainImageViews[i],
            };
        }
        else {

            attachments = {
                swapchainImageViews[i],
                m_depthAttachment->m_imageView,
                m_shadowRenderPass->getShadowAttachment(),
            };
        }

        vk::Extent2D extent = getRenderPassExtent();

        vk::FramebufferCreateInfo framebufferInfo{
            .renderPass = m_renderPass, //Renderpass that is compatible with the framebuffer
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
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


