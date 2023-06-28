#include "MainRenderPass.h"

//Always create this render pass after creating the shadow render pass
MainRenderPass::MainRenderPass(VulkanContext* context, Camera* camera, ShadowCascadeRenderPass* shadowRenderPass) : VulkanRenderPass(context)
{
    std::vector<vk::ImageView> swapchainImageViews = m_context->getSwapchainImageViews();
    m_framebuffers.resize(m_context->getSwapchainImagesCount());

    m_shadowRenderPass = shadowRenderPass;
    m_camera = camera;

    //TODO Remove
    material = (new Material(m_context))
        ->setBaseColor(glm::vec4(1.f, 1.f, 1.f, 1.f));
}

MainRenderPass::~MainRenderPass()
{
    vk::Device device = m_context->getDevice();
    device.destroyDescriptorPool(m_shadowDescriptorPool);
    device.destroyDescriptorSetLayout(m_shadowDescriptorSetLayout);
    delete m_defaultTexture;
    delete m_defaultNormalMap;
    delete material;
    device.destroySampler(m_shadowMapSampler);
    cleanAttachments();
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_context->getAllocator().destroyBuffer(m_generalUniformBuffers[i], m_generalUniformBuffersAllocations[i]);
        m_context->getAllocator().destroyBuffer(m_lightUniformBuffers[i], m_lightUniformBuffersAllocations[i]);
        m_context->getAllocator().destroyBuffer(m_materialUniformBuffer[i], m_materialUniformBufferAllocations[i]);
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
        std::array<vk::DescriptorPoolSize, 4> poolSizes;
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[2].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[3].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_TEXTURE_COUNT; //Dynamic Indexing

        vk::DescriptorPoolCreateInfo poolInfo{
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
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

    vk::DescriptorSetLayoutBinding lightUboLayoutBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutBinding materialUboLayoutBinding{
        .binding = 2,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutBinding samplerLayoutBinding{
        .binding = 3,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = MAX_TEXTURE_COUNT,  //Dynamic Indexing
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutBinding bindings[4] = { uboLayoutBinding, lightUboLayoutBinding, materialUboLayoutBinding, samplerLayoutBinding };
    //Descriptor indexing
    vk::DescriptorBindingFlags bindingFlags[4];
    bindingFlags[3] = vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount; //Necessary for Dynamic indexing (VK_EXT_descriptor_indexing)

    vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{
        .bindingCount = 4,
        .pBindingFlags = bindingFlags,
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .pNext = &bindingFlagsCreateInfo,
        .bindingCount = 4,
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

//adds the texture info to the texture image info and sets the right Id to the mesh
static void appendImageInfo(std::vector<vk::DescriptorImageInfo>& textureImageInfo, vk::Sampler sampler, VulkanImage* image, uint32_t& id, uint32_t& textureId)
{
    vk::DescriptorImageInfo imageInfo{
              .sampler = sampler,
              .imageView = image->m_imageView,
              .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    textureImageInfo.push_back(imageInfo);
    id = textureId; //Attributes the texture id to the mesh
    textureId++;
}

//returns are vector of DescriptorImageInfo, containing the albedo and normal map texture information from the Models' TexturedMeshes
std::vector<vk::DescriptorImageInfo> MainRenderPass::generateTextureImageInfo(VulkanScene* scene)
{
    std::vector<vk::DescriptorImageInfo> textureImageInfo;
    uint32_t textureId = 0;
    for (auto& model : scene->m_models) {
        for (auto& texturedMesh : model->getMeshes()) {
            if (texturedMesh.textureImage != nullptr && !texturedMesh.textureImage->hasLoadingFailed())
            {
                appendImageInfo(textureImageInfo, Material::s_baseColorSampler, texturedMesh.textureImage, texturedMesh.textureId, textureId);
            }
            else
            {
                //No texture applies a default texture
                appendImageInfo(textureImageInfo, Material::s_baseColorSampler, m_defaultTexture, texturedMesh.textureId, textureId);
            }
            if (texturedMesh.normalMapImage != nullptr && !texturedMesh.normalMapImage->hasLoadingFailed())
            {
                appendImageInfo(textureImageInfo, Material::s_baseColorSampler, texturedMesh.normalMapImage, texturedMesh.normalMapId, textureId);
            }
            else
            {
                //No texture applies a default texture
                appendImageInfo(textureImageInfo, Material::s_baseColorSampler, m_defaultNormalMap, texturedMesh.normalMapId, textureId);
            }

        }
    }
    return textureImageInfo;
}

//Creates the descriptor set that has the general/camera ubo, the light ubo array and the texture array
void MainRenderPass::createMainDescriptorSet(VulkanScene* scene)
{
    //Creates a vector of descriptorImageInfo from the scene's textures
    std::vector<vk::DescriptorImageInfo> textureImageInfos = generateTextureImageInfo(scene);

    m_mainDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_mainDescriptorSetLayout);

    /* Dynamic Descriptor Counts */
    uint32_t textureMaxCount = MAX_TEXTURE_COUNT;
    std::vector<uint32_t> textureMaxCounts(MAX_FRAMES_IN_FLIGHT, textureMaxCount);
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
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo generalUboBufferInfo{
            .buffer = m_generalUniformBuffers[i],
            .offset = 0,
            .range = sizeof(GeneralUniformBufferObject)
        };

        vk::DescriptorBufferInfo lightUboBufferInfo{
             .buffer = m_lightUniformBuffers[i],
             .offset = 0,
             .range = sizeof(LightUBO) * MAX_LIGHT_COUNT
        };

        vk::DescriptorBufferInfo materialUboBufferInfo{
             .buffer = m_materialUniformBuffer[i],
             .offset = 0,
             .range = sizeof(MaterialUBO),
        };

     

        std::array<vk::WriteDescriptorSet, 4> descriptorWrites;
        descriptorWrites[0].dstSet = m_mainDescriptorSet[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &generalUboBufferInfo;

        descriptorWrites[1].dstSet = m_mainDescriptorSet[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &lightUboBufferInfo;

        descriptorWrites[2].dstSet = m_mainDescriptorSet[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &materialUboBufferInfo;

        descriptorWrites[3].dstSet = m_mainDescriptorSet[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].descriptorCount = textureImageInfos.size();
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[3].pImageInfo = textureImageInfos.data();


        try {
            m_context->getDevice().updateDescriptorSets(descriptorWrites, nullptr);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor sets");
        }
    }
}

//Creates the descriptor set that has the shadow map textures
void MainRenderPass::createShadowDescriptorSet(VulkanScene* scene)
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

void MainRenderPass::createDescriptorSets(VulkanScene* scene)
{
    createMainDescriptorSet(scene);
    createShadowDescriptorSet(scene);
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
       .vertPath = "shaders/vertexPBR.spv",
       .fragPath = "shaders/fragmentPBR.spv",
    };

    m_mainPipeline = new VulkanPipeline(m_context, pipelineInfo, m_pipelineLayout, m_renderPass, getRenderPassExtent());

}

vk::Extent2D MainRenderPass::getRenderPassExtent()
{
    return m_context->getSwapchainExtent();
}

//Records commands for drawing ImGUI debug window
void MainRenderPass::renderImGui(vk::CommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();


    //imgui commands
    double framerate = ImGui::GetIO().Framerate;
    ImGui::Begin("Renderer Performance", &m_hideImGui);
    ImGui::SetWindowSize(ImVec2(400.f, 250.f));
    ImGui::SetWindowPos(ImVec2(10.f, 10.f));
    ImGui::Text("Framerate: %f", framerate);
    ImGui::SliderFloat("Cascade Splitting Lambda: ", &m_shadowRenderPass->m_cascadeSplitLambda, 0.f, 1.f, "%.2f", 0);
    ImGui::SliderFloat("Shadowmap Blend Width: ", &m_shadowRenderPass->m_shadowMapsBlendWidth, 0.f, 1.f, "%.2f", 0);
    ImGui::Separator();
    ImGui::Text("Material !");
    ImGui::SliderFloat("Metallic Factor: ", &metallicFactorGui, 0, 1, "%.2f");
    ImGui::SliderFloat("Roughness Factor: ", &roughnessFactorGui, 0.001f, 1, "%.2f");
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
    createUniformBuffers();
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

//updates uniform buffer for camera/general uniform buffer
void MainRenderPass::updateGeneralUniformBuffer(uint32_t currentFrame) {
    vk::Extent2D extent = getRenderPassExtent();

    //Model View Proj
    GeneralUniformBufferObject ubo{};
    ubo.view = m_camera->getViewMatrix();
    ubo.proj = m_camera->getProjMatrix(m_context);
    ubo.cameraPos = m_camera->getCameraPos();
    ubo.time = m_context->getTime().elapsedSinceStart;
    ubo.shadowMapsBlendWidth = m_shadowRenderPass->m_shadowMapsBlendWidth;

    //Get the cascade view/proj matrices and frustrum splits previously calculated in the shadowRenderPass
    CascadeUniformObject cascadeUbo = m_shadowRenderPass->getCurrentUbo(currentFrame);

    for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
    {
        ubo.cascadeSplits[i] = cascadeUbo.cascadeSplits[i];
        ubo.cascadeViewProj[i] = cascadeUbo.cascadeViewProjMat[i];
    }

    void* data = m_context->getAllocator().mapMemory(m_generalUniformBuffersAllocations[currentFrame]);
    memcpy(data, &ubo, sizeof(GeneralUniformBufferObject));
    m_context->getAllocator().unmapMemory(m_generalUniformBuffersAllocations[currentFrame]);
}

//Updates uniform buffer for Light uniform data
void MainRenderPass::updateLightUniformBuffer(uint32_t currentFrame, std::vector<VulkanScene*> scenes) {
    std::vector<Light*> lights = scenes[0]->getLights();
    std::array<LightUBO, MAX_LIGHT_COUNT> lightsUbo;
    for (uint32_t i = 0; i < MAX_LIGHT_COUNT; i++)
    {
        if (i < lights.size())
        {
            LightUBO ubo = lights[i]->getUniformData();
            lightsUbo[i] = ubo;
        }
        else {
            lightsUbo[i] = LightUBO{};
        }

    }

    void* data = m_context->getAllocator().mapMemory(m_lightUniformBuffersAllocations[currentFrame]);
    memcpy(data, lightsUbo.data(), sizeof(LightUBO)*lightsUbo.size());
    m_context->getAllocator().unmapMemory(m_lightUniformBuffersAllocations[currentFrame]);

}

void MainRenderPass::updateMaterialUniformBuffer(uint32_t currentFrame, std::vector<VulkanScene*> scenes)
{
    MaterialUBO materialUbo = material->getUBO();
    materialUbo.metallicFactor = metallicFactorGui;
    materialUbo.roughnessFactor = roughnessFactorGui;

    void* data = m_context->getAllocator().mapMemory(m_materialUniformBufferAllocations[currentFrame]);
    memcpy(data, &materialUbo, sizeof(MaterialUBO));
    m_context->getAllocator().unmapMemory(m_materialUniformBufferAllocations[currentFrame]);
}


//Populates the uniform buffers for rendering
void MainRenderPass::updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes)
{
    updateGeneralUniformBuffer(currentFrame);
    updateLightUniformBuffer(currentFrame, scenes);
    updateMaterialUniformBuffer(currentFrame, scenes);
    
}

//Creates the default albedo and normal map textures
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
    m_defaultTexture = new VulkanImage(m_context, imageParams, imageViewParams, c_defaultTexturePath);

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
    m_defaultNormalMap = new VulkanImage(m_context, imageParams, imageViewParams, c_defaultNormalMapPath);
}

void MainRenderPass::createUniformBuffers() {
    //General UBO
    { 
        vk::DeviceSize bufferSize = sizeof(GeneralUniformBufferObject);

        m_generalUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_generalUniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::tie(m_generalUniformBuffers[i], m_generalUniformBuffersAllocations[i]) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);
        }
    }
    {
        vk::DeviceSize bufferSize = sizeof(LightUBO)*MAX_LIGHT_COUNT;

        m_lightUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_lightUniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::tie(m_lightUniformBuffers[i], m_lightUniformBuffersAllocations[i]) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);
        }
    }

    {
        vk::DeviceSize bufferSize = sizeof(MaterialUBO);

        m_materialUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        m_materialUniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::tie(m_materialUniformBuffer[i], m_materialUniformBufferAllocations[i]) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);
        }
    }

}


//Creates the sampler used for shadow mapping sampling
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