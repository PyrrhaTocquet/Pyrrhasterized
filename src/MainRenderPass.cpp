#include "MainRenderPass.h"

//Always create this render pass after creating the shadow render pass
MainRenderPass::MainRenderPass(VulkanContext *context, ShadowCascadeRenderPass *shadowRenderPass, DepthPrePass *depthPrePass) : VulkanRenderPass(context)
{
    std::vector<vk::ImageView> swapchainImageViews = m_context->getSwapchainImageViews(); // REMOVE ? useless ?
    m_framebuffers.resize(m_context->getSwapchainImagesCount());

    m_shadowRenderPass = shadowRenderPass;
    m_depthPrePass = depthPrePass;
}

MainRenderPass::~MainRenderPass()
{
    vk::Device device = m_context->getDevice();
    device.destroyDescriptorPool(m_materialDescriptorPool);
    device.destroyDescriptorSetLayout(m_materialDescriptorSetLayout);
    device.destroySampler(m_shadowMapSampler);
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
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eAttachmentOptimal,
        .finalLayout = vk::ImageLayout::eAttachmentOptimal,
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
}

void MainRenderPass::cleanAttachments()
{
    if (ENABLE_MSAA)delete m_colorAttachment;
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
    //General Data, Lights, Shadow Maps, Textures
    {
        std::array<vk::DescriptorPoolSize, 4> poolSizes;
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[2].type = vk::DescriptorType::eCombinedImageSampler;
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

void MainRenderPass::createDescriptorSetLayout()
{
    vk::Device device = m_context->getDevice();

    //Set 0: General Uniform data, Light Data, Shadow Map, all textures
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutBinding lightUboLayoutBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutBinding shadowSamplerLayoutBinding{
        .binding = 2,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutBinding samplerLayoutBinding{
        .binding = 3,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = MAX_TEXTURE_COUNT,  //Dynamic Indexing
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutBinding bindings[4] = { uboLayoutBinding, lightUboLayoutBinding, shadowSamplerLayoutBinding, samplerLayoutBinding };
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

//Creates the descriptor set that has the general/camera ubo, the light ubo array, the shadow map and the texture array
void MainRenderPass::createMainDescriptorSet(VulkanScene* scene)
{
    //Creates a vector of descriptorImageInfo from the scene's textures
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

        vk::DescriptorBufferInfo lightUboBufferInfo
        {
             .buffer = scene->getLightUniformBuffer(currentFrame).m_Buffer,
             .offset = 0,
             .range = sizeof(LightUBO) * MAX_LIGHT_COUNT
        };

        vk::DescriptorImageInfo shadowImageInfo{
            .sampler = m_shadowMapSampler,
            .imageView = m_shadowRenderPass->getShadowAttachment(),
            .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
        };
     

        std::array<vk::WriteDescriptorSet, 4> descriptorWrites;
        descriptorWrites[0].dstSet = m_mainDescriptorSet[currentFrame];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &generalUboBufferInfo;

        descriptorWrites[1].dstSet = m_mainDescriptorSet[currentFrame];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &lightUboBufferInfo;

        descriptorWrites[2].dstSet = m_mainDescriptorSet[currentFrame];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &shadowImageInfo;

        descriptorWrites[3].dstSet = m_mainDescriptorSet[currentFrame];
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

//Creates the descriptor set for the material data
void MainRenderPass::createMaterialDescriptorSet(VulkanScene* scene)
{
// TODO Refactor in scene !
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

   // TODO THIS
    for (uint32_t currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++) {
        std::vector<vk::DescriptorBufferInfo> materialBufferInfos;

        //Creating and filling the Uniform Buffers for each material
        vk::DeviceSize bufferSize = sizeof(MaterialUBO);

        materialBufferInfos.resize(scene->m_materialCount);
        for (size_t i = 0; i < materialBufferInfos.size(); i++) {
            
            //filling the material buffer infos
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = scene->getMaterialUniformBuffer(currentFrame, i).m_Buffer,
                .offset = 0,
                .range = bufferSize
            };
            materialBufferInfos[i] = bufferInfo;
        }

        vk::WriteDescriptorSet descriptorWriteInfo{
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

void MainRenderPass::createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo> textureImageInfos)
{
    createMainDescriptorSet(scene);
    createMaterialDescriptorSet(scene);
}

void MainRenderPass::createPipelineLayout(vk::DescriptorSetLayout geometryDescriptorSetLayout)
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

void MainRenderPass::createDefaultPipeline()
{
    PipelineInfo pipelineInfo{
        .taskShaderPath = "shaders/taskShell.spv",
       .meshShaderPath = "shaders/meshPBR.spv",
       .fragShaderPath = "shaders/fragmentPBR.spv",
       .depthWriteEnable = VK_FALSE,
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
    ImGui::SetWindowSize(ImVec2(400.f, 500.f));
    ImGui::SetWindowPos(ImVec2(10.f, 10.f));
    
    ImGui::Text("Statistics:");
    ImGui::Text("Framerate: %f", framerate);
    ImGui::Text("----------");


    ImGui::Text("Shadows:");
    ImGui::SliderFloat("Cascade Splitting Lambda: ", &m_shadowRenderPass->m_cascadeSplitLambda, 0.001f, .999f, "%.2f", 0);
    ImGui::SliderFloat("Shadowmap Blend Width: ", &m_shadowRenderPass->m_shadowMapsBlendWidth, 0.001f, 0.999f, "%.2f", 0);
    ImGui::Text("----------");
    
    ImGui::Text("Shell Texturing");
    int currentChoice = 0;
    std::array<std::string, 11> shellCountList = {"1", "2", "4", "8", "16", "32", "64", "128", "256", "512", "1024"};
    int k = 0;
    if (ImGui::ListBoxHeader("Shell Count"))
    {
        for (auto shellCountStr : shellCountList)
        {
            if (ImGui::Selectable(shellCountStr.c_str(), selectedId == k ? true : false))
            {
                selectedId = k;
                shellCount = pow(2, selectedId);
            }
            k++;
        }
        ImGui::ListBoxFooter();
    }
   
    
    ImGui::SliderFloat("Hair Length: ", &hairLength, 0.001f, .1f, "%.5f", 0);
    ImGui::SliderFloat("Hair Gravity: ", &gravityFactor, 0.001f, 0.1f, "%.5f", 0);
    ImGui::SliderFloat("Hair Density: ", &hairDensity, 50.f, 3000.f, "%.5f", 0);
    
    ImGui::End();

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void MainRenderPass::createFramebuffer() {
    std::vector<vk::ImageView> swapchainImageViews = m_context->getSwapchainImageViews();
    std::vector<vk::ImageView> attachments;

    vk::ImageView depthPrePassImageView = m_depthPrePass->getDepthAttachment()->m_imageView;
    for (size_t i = 0; i < m_framebuffers.size(); i++) {
        if (ENABLE_MSAA)
        {
            attachments = { //Order corresponds to the attachment references
            m_colorAttachment->m_imageView,
            depthPrePassImageView,
            swapchainImageViews[i],
            };
        }
        else {

            attachments = {
                swapchainImageViews[i],
                depthPrePassImageView
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
    pushConstant.shellCount = shellCount;
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_mainPipeline->getPipeline()); //Only one main draw pipeline per frame in this renderer
    //Draws each scene
    for (auto& scene : scenes)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, { scene->getGeometryDescriptorSet() , m_mainDescriptorSet[m_currentFrame], m_materialDescriptorSet[m_currentFrame]}, nullptr);
        scene->draw(commandBuffer, m_currentFrame, m_pipelineLayout, pushConstant);
    }
    
    //renderImGui(commandBuffer);
    commandBuffer.endRenderPass();
}

void MainRenderPass::createPipelineRessources() {
    createShadowMapSampler();
}

void MainRenderPass::createPushConstantsRanges()
{
    m_pushConstant = vk::PushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = 128,
    };

}

void MainRenderPass::updateMaterialUniformBuffer(uint32_t currentFrame, std::vector<VulkanScene*> scenes)
{
    /*MaterialUBO materialUbo = material->getUBO();
    materialUbo.metallicFactor = metallicFactorGui;
    materialUbo.roughnessFactor = roughnessFactorGui;

    void* data = m_context->getAllocator()->mapMemory(m_materialUniformBufferAllocations[currentFrame]);
    memcpy(data, &materialUbo, sizeof(MaterialUBO));
    m_context->getAllocator()->unmapMemory(m_materialUniformBufferAllocations[currentFrame]);
    */
}

//Populates the uniform buffers for rendering
void MainRenderPass::updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes)
{
    
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