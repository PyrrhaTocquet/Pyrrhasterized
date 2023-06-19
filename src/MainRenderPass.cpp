#include "MainRenderPass.h"

//Always create this render pass after creating the shadow render pass
MainRenderPass::MainRenderPass(VulkanContext* context, Camera* camera, ShadowCascadeRenderPass* shadowRenderPass) : VulkanRenderPass(context)
{
    std::vector<vk::ImageView> swapchainImageViews = m_context->getSwapchainImageViews();
    m_framebuffers.resize(m_context->getSwapchainImagesCount());

    m_shadowRenderPass = shadowRenderPass;
    m_camera = camera;
}

MainRenderPass::~MainRenderPass()
{
    vk::Device device = m_context->getDevice();
    device.destroyDescriptorPool(m_shadowDescriptorPool);
    device.destroyDescriptorSetLayout(m_shadowDescriptorSetLayout);
    delete m_defaultTexture;
    delete m_defaultNormalMap;
    device.destroySampler(m_textureSampler);
    device.destroySampler(m_shadowMapSampler);
    cleanAttachments();
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_context->getAllocator().destroyBuffer(m_uniformBuffers[i], m_uniformBuffersAllocations[i]);
    }
    
}

void MainRenderPass::createRenderPass()
{
    vk::SampleCountFlagBits msaaSampleCount = m_context->getMaxUsableSampleCount();

    //MSAA color target
    vk::AttachmentDescription colorDescription{
        .format = m_context->getSwapchainFormat(),
        .samples = ENABLE_MSAA ? msaaSampleCount : vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = ENABLE_MSAA ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore, //Best practice with multisampled images is to make the best of lazy allocation with don't care
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = ENABLE_MSAA ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
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
        .attachment = 2,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };


    vk::SubpassDescription subpass = vk::SubpassDescription{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = ENABLE_MSAA ? &colorAttachmentResolveRef : nullptr,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    vk::SubpassDependency dependencyColorAttachment{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
    };
    vk::SubpassDependency dependencyDepthAttachment{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    };



    std::vector<vk::AttachmentDescription> attachments{ colorDescription, depthDescription };
    if (ENABLE_MSAA)attachments.push_back(colorDescriptionResolve);

    std::vector<vk::SubpassDependency> dependencies{ dependencyColorAttachment, dependencyDepthAttachment };
    vk::RenderPassCreateInfo renderPassInfo{
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
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
    m_context->getDevice().destroyPipeline(m_mainPipeline->getPipeline());
    m_mainPipeline->recreatePipeline(getRenderPassExtent());
    cleanAttachments();
    cleanFramebuffer();
    createAttachments();
    createFramebuffer();
}

void MainRenderPass::createDescriptorPool()
{
    vk::Device device = m_context->getDevice();
    //MVP UBO + TEXTURES
    {
        vk::DescriptorPoolSize poolSizes[2];
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_TEXTURE_COUNT; //Dynamic Indexing

        vk::DescriptorPoolCreateInfo poolInfo{
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = static_cast<uint32_t>(2),
            .pPoolSizes = poolSizes,
        };

        try {
            m_mainDescriptorPool = device.createDescriptorPool(poolInfo);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor pool");
        }
    }
    //SHADOW MAP INPUT
    {
        vk::DescriptorPoolSize shadowPoolSize;
        shadowPoolSize.type = vk::DescriptorType::eCombinedImageSampler; 
        shadowPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        vk::DescriptorPoolCreateInfo shadowPoolInfo{
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = 1,
            .pPoolSizes = &shadowPoolSize,
        };

        try {
            m_shadowDescriptorPool = device.createDescriptorPool(shadowPoolInfo);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor pool");
        }
    }

}

void MainRenderPass::createDescriptorSetLayout()
{
    vk::Device device = m_context->getDevice();

    //Set 0: Transforms Uniform Buffer, Texture sampler
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
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

    //Set 1: Shadow map sampler
    vk::DescriptorSetLayoutBinding shadowSamplerLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutCreateInfo shadowLayoutInfo{
        .bindingCount = 1,
        .pBindings = &shadowSamplerLayoutBinding,
    };

    try {
        m_shadowDescriptorSetLayout = device.createDescriptorSetLayout(shadowLayoutInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor set layout");
    }
}

void MainRenderPass::createDescriptorSet(VulkanScene* scene)
{

    //Creates a vector of descriptorImageInfo from the scene's textures
    std::vector<vk::DescriptorImageInfo> textureImageInfo;
    uint32_t textureId = 0;
    for (auto& model : scene->m_models) {
        for (auto& texturedMesh : model->getMeshes()) {
            if (texturedMesh.textureImage != nullptr && !texturedMesh.textureImage->hasLoadingFailed())
            {
                vk::DescriptorImageInfo imageInfo{
                .sampler = m_textureSampler,
                .imageView = texturedMesh.textureImage->m_imageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                };
                textureImageInfo.push_back(imageInfo);
                texturedMesh.textureId = textureId; //Attributes the texture id to the mesh
                textureId++;

            }
            else
            {
                //No texture applies a default texture
                vk::DescriptorImageInfo imageInfo{
               .sampler = m_textureSampler,
               .imageView = m_defaultTexture->m_imageView,
               .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                };
                //Default texture if no texture
                textureImageInfo.push_back(imageInfo);
                texturedMesh.textureId = textureId;
                textureId++;
            }
            if (texturedMesh.normalMapImage != nullptr && !texturedMesh.normalMapImage->hasLoadingFailed())
            {
                vk::DescriptorImageInfo imageInfo{
                .sampler = m_textureSampler,
                .imageView = texturedMesh.normalMapImage->m_imageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                };
                textureImageInfo.push_back(imageInfo);
                texturedMesh.normalMapId = textureId; //Attributes the texture id to the mesh
                textureId++;

            }
            else
            {
                //No texture applies a default texture
                vk::DescriptorImageInfo imageInfo{
               .sampler = m_textureSampler,
               .imageView = m_defaultNormalMap->m_imageView,
               .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                };
                //Default texture if no texture
                textureImageInfo.push_back(imageInfo);
                texturedMesh.normalMapId = textureId;
                textureId++;
            }

        }
    }

    m_mainDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_mainDescriptorSetLayout);

    /* Dynamic Descriptor Counts */
    uint32_t textureMaxCount = MAX_TEXTURE_COUNT;
    std::vector<uint32_t> textureMaxCounts(MAX_FRAMES_IN_FLIGHT, textureMaxCount);
    vk::DescriptorSetVariableDescriptorCountAllocateInfo setCounts{
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pDescriptorCounts = textureMaxCounts.data(),
    };

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
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = m_uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        std::vector<vk::WriteDescriptorSet> descriptorWrites(2);
        descriptorWrites[0].dstSet = m_mainDescriptorSet[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].dstSet = m_mainDescriptorSet[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorCount = textureImageInfo.size();
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].pImageInfo = textureImageInfo.data();


        try {
            m_context->getDevice().updateDescriptorSets(descriptorWrites, nullptr);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor sets");
        }
    }

    {
        m_shadowDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
        std::vector<vk::DescriptorSetLayout> shadowLayouts(MAX_FRAMES_IN_FLIGHT, m_shadowDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo allocInfo = {
             .descriptorPool = m_shadowDescriptorPool,
             .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
             .pSetLayouts = shadowLayouts.data()
        };

        try {
            m_shadowDescriptorSet = m_context->getDevice().allocateDescriptorSets(allocInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("could not allocate descriptor sets");
        }

        //SHADOW TODO REFACTOR
        vk::DescriptorImageInfo shadowTextureImageInfo{
          .sampler = m_shadowMapSampler,
          .imageView = m_shadowRenderPass->getShadowAttachment(),
          .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
        };

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {


            vk::WriteDescriptorSet writeDescriptorSet;
            writeDescriptorSet.dstSet = m_shadowDescriptorSet[i];
            writeDescriptorSet.dstBinding = 0;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            writeDescriptorSet.pImageInfo = &shadowTextureImageInfo;

            try {
                m_context->getDevice().updateDescriptorSets(writeDescriptorSet, nullptr);
            }
            catch (vk::SystemError err)
            {
                throw std::runtime_error("could not create descriptor sets");
            }
        }
    }
}

void MainRenderPass::createPipelineLayout()
{
    std::array<vk::DescriptorSetLayout, 2> layouts = { m_mainDescriptorSetLayout, m_shadowDescriptorSetLayout };

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

void MainRenderPass::createDefaultPipeline()
{
    PipelineInfo pipelineInfo{
       .vertPath = "shaders/vertexTextureCSM.spv",
       .fragPath = "shaders/fragmentTextureCSM.spv",
    };

    m_mainPipeline = new VulkanPipeline(m_context, pipelineInfo, m_pipelineLayout, m_renderPass, getRenderPassExtent());

}

vk::Extent2D MainRenderPass::getRenderPassExtent()
{
    return m_context->getSwapchainExtent();
}

void MainRenderPass::renderImGui(vk::CommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();


    //imgui commands
    double framerate = ImGui::GetIO().Framerate;
    ImGui::Begin("Renderer Performance", &m_hideImGui);
    ImGui::SetWindowSize(ImVec2(300.f, 100.f));
    ImGui::SetWindowPos(ImVec2(10.f, 10.f));
    ImGui::Text("Framerate: %f", framerate);
    ImGui::SliderFloat("Cascade Splitting Lambda: ", &m_shadowRenderPass->m_cascadeSplitLambda, 0.f, 1.f, "%.2f", 0);
    ImGui::SliderFloat("Shadowmap Blend Width: ", &m_shadowRenderPass->m_shadowMapsBlendWidth, 0.f, 1.f, "%.2f", 0);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
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
            swapchainImageViews[i],
            };
        }
        else {

            attachments = {
                swapchainImageViews[i],
                m_depthAttachment->m_imageView,
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

void MainRenderPass::drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes)
{
    vk::RenderPassBeginInfo renderPassInfo{
       .renderPass = m_renderPass,
       .framebuffer = m_framebuffers[swapchainImageIndex],
       .renderArea = {
           .offset = {0, 0},
           .extent = getRenderPassExtent(),
       },
       .clearValueCount = static_cast<uint32_t>(MAIN_CLEAR_VALUES.size()),
       .pClearValues = MAIN_CLEAR_VALUES.data(),
    };
    ModelPushConstant pushConstant;
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_mainPipeline->getPipeline()); //Only one main draw pipeline per frame in this renderer
    //Draws each scene
    for (auto& scene : scenes)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, { m_mainDescriptorSet[m_currentFrame], m_shadowDescriptorSet[m_currentFrame]}, nullptr);
        scene->draw(commandBuffer, m_currentFrame, m_pipelineLayout, pushConstant);
    }
    
    renderImGui(commandBuffer);
    commandBuffer.endRenderPass();
}

void MainRenderPass::createPipelineRessources() {
    createDefaultTextures();
    createUniformBuffer();
    createTextureSampler();
    createShadowMapSampler();
}

void MainRenderPass::createPushConstantsRanges()
{
    m_pushConstant = vk::PushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = 128,
    };

}

void MainRenderPass::updatePipelineRessources(uint32_t currentFrame)
{
    //Time since rendering start
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - startTime).count();
    vk::Extent2D extent = getRenderPassExtent();


    //Model View Proj
    UniformBufferObject ubo{};
    ubo.view = m_camera->getViewMatrix();
    ubo.proj = m_camera->getProjMatrix(m_context);
    ubo.cameraPos = m_camera->getCameraPos();
    ubo.shadowMapsBlendWidth = m_shadowRenderPass->m_shadowMapsBlendWidth;
    CascadeUniformObject cascadeUbo = m_shadowRenderPass->getCurrentUbo(currentFrame);

    for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
    {
        ubo.cascadeSplits[i] = cascadeUbo.cascadeSplits[i];
        ubo.cascadeViewProj[i] = cascadeUbo.cascadeViewProjMat[i];
    }

    void* data = m_context->getAllocator().mapMemory(m_uniformBuffersAllocations[currentFrame]);
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    m_context->getAllocator().unmapMemory(m_uniformBuffersAllocations[currentFrame]);

}


void MainRenderPass::createDefaultTextures() {
    VulkanImageParams imageParams
    {
        .numSamples = vk::SampleCountFlagBits::e1,
        .format = vk::Format::eR8G8B8A8Srgb,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eSampled,
    };

    VulkanImageViewParams imageViewParams{
        .aspectFlags = vk::ImageAspectFlagBits::eColor,
    };
    m_defaultTexture = new VulkanImage(m_context, imageParams, imageViewParams, "assets/defaultTexture.png");

    imageParams = VulkanImageParams
    {
        .numSamples = vk::SampleCountFlagBits::e1,
        .format = vk::Format::eR8G8B8A8Unorm,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eSampled,
    };

    imageViewParams = VulkanImageViewParams{
        .aspectFlags = vk::ImageAspectFlagBits::eColor,
    };
    m_defaultNormalMap = new VulkanImage(m_context, imageParams, imageViewParams, "assets/defaultNormalMap.png");
}

void MainRenderPass::createUniformBuffer() {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::tie(m_uniformBuffers[i], m_uniformBuffersAllocations[i]) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);
    }
}

void MainRenderPass::createTextureSampler() {
    vk::PhysicalDeviceProperties properties = m_context->getProperties();

    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear, //Linear filtering
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE, //Anisotropic Filtering
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy, //Max texels samples to calculate the final color
        .compareEnable = VK_FALSE, //If enabled, texels will be compared to a value and the result of that compararison is used in filtering operations
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = vk::BorderColor::eIntOpaqueBlack, //only useful when clamping
        .unnormalizedCoordinates = VK_FALSE, //Normalized coordinates allow to change texture resolution for the same UVs
    };

    try {
        m_textureSampler = m_context->getDevice().createSampler(samplerInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("failed to create texture sampler");
    }

}

void MainRenderPass::createShadowMapSampler() {
    vk::PhysicalDeviceProperties properties = m_context->getProperties();

    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear, //Linear filtering
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .mipLodBias = 0.0f,
        .maxAnisotropy = 1.0f, //Max texels samples to calculate the final color
        .minLod = 0.0f,
        .maxLod = 1.0f,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite, //only useful when clamping
        .unnormalizedCoordinates = VK_FALSE, //Normalized coordinates allow to change texture resolution for the same UVs
    };

    try {
        m_shadowMapSampler = m_context->getDevice().createSampler(samplerInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("failed to create texture sampler");
    }
}