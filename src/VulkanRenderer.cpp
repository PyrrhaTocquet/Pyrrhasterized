#include "VulkanRenderer.h"

#pragma region CONSTRUCTORS_DESTRUCTORS
VulkanRenderer::VulkanRenderer(VulkanContext* context)
{
     //Retrieving important values and references from VulkanContext
    m_context = context;
    m_device = context->getDevice();
    m_allocator = context->getAllocator();
    m_msaaSampleCount = context->getMaxUsableSampleCount();
    
    //Uniform buffers creation
    createPushConstantRanges();
    createDescriptorSetLayout();
    createDescriptorPools();
    createDescriptorObjects();

    //Rendering pipeline creation
    createRenderPasses();
    createPipelineLayout();
    createMainGraphicsPipeline("shaders/vertexTexture.spv", "shaders/fragmentTexture.spv");
    createShadowGraphicsPipeline("shaders/vertexShadow.spv", "shaders/fragmentShadow.spv");
    createFramebuffers();
   
    //Command execution related objects
    createCommandBuffers();
    createSyncObjects();
    createShadowDescriptorSets();
    
    //IMGUI
    ImGui_ImplVulkan_InitInfo initInfo = m_context->getImGuiInitInfo();
    initInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(ENABLE_MSAA ? m_msaaSampleCount : vk::SampleCountFlagBits::e1);
    ImGui_ImplVulkan_Init(&initInfo, m_renderPasses[RenderPassesId::MainRenderPassId]->getRenderPass());
    m_device.waitIdle();

    //execute a gpu command to upload imgui font textures
    vk::CommandBuffer cmd = m_context->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    m_context->endSingleTimeCommands(cmd);

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    //CAMERA
    m_camera = new Camera(m_context);
    registerEntity(m_camera);

}

VulkanRenderer::~VulkanRenderer()
{
    delete m_camera;

    delete m_defaultTexture;
    delete m_defaultNormalMap;

    m_device.destroySampler(m_textureSampler);
    m_device.destroyDescriptorPool(m_mainDescriptorPool);
    m_device.destroyDescriptorPool(m_shadowDescriptorPool);
    m_device.destroyDescriptorSetLayout(m_mainDescriptorSetLayout);
    m_device.destroyDescriptorSetLayout(m_shadowDescriptorSetLayout);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_allocator.destroyBuffer(m_uniformBuffers[i], m_uniformBuffersAllocations[i]);

        m_device.destroySemaphore(m_renderFinishedSemaphores[i]);
        m_device.destroySemaphore(m_imageAvailableSemaphores[i]);
        m_device.destroyFence(m_inFlightFences[i]);
    }


    m_device.freeCommandBuffers(m_context->getCommandPool(), m_commandBuffers);
    for (auto pipeline : m_pipelines) {
        m_device.destroyPipeline(pipeline.pipeline);
    }
    m_device.destroyPipeline(m_shadowPipeline.pipeline);

    m_device.destroyPipelineLayout(m_pipelineLayout);
    
    for (auto& renderPass : m_renderPasses)
    {
        delete renderPass;
    }
}
#pragma endregion

#pragma region PIPELINE
//adds a pipeline to the pipeline list
void VulkanRenderer::addPipeline(VulkanPipeline pipeline) {
    m_pipelines.push_back(pipeline);
}

//creates the main Pipeline Layout used for all pipelines
void VulkanRenderer::createPipelineLayout() {

    std::array<vk::DescriptorSetLayout, 2> layouts = { m_mainDescriptorSetLayout, m_shadowDescriptorSetLayout };
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
       .setLayoutCount = layouts.size(),
       .pSetLayouts = layouts.data(),
       .pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size()),
       .pPushConstantRanges = m_pushConstantRanges.data(),
    };

    try {
        m_pipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void VulkanRenderer::createShadowGraphicsPipeline(const char* vertShaderCodePath, const char* fragShaderCodePath) {

    PipelineInfo pipelineInfo{
    .vertPath = vertShaderCodePath,
    .fragPath = fragShaderCodePath,
    .cullmode = vk::CullModeFlagBits::eNone,
    .renderPassId = RenderPassesId::ShadowMappingPassId,
    .isMultisampled = false,
    };

    m_shadowPipeline = createPipeline(pipelineInfo);

}

//Creates the main graphics pipeline
void VulkanRenderer::createMainGraphicsPipeline(const char* vertShaderCodePath,const char* fragShaderCodePath)
{
    PipelineInfo pipelineInfo{
        .vertPath = vertShaderCodePath,
        .fragPath = fragShaderCodePath,
    };

    VulkanPipeline pipeline = createPipeline(pipelineInfo);
    m_pipelines.push_back(pipeline);

}

VulkanPipeline VulkanRenderer::createPipeline(PipelineInfo pipelineInfo) {
    if (!ENABLE_MSAA)
    {
        pipelineInfo.isMultisampled = false;
    }

    auto vertShaderCode = vkTools::readFile(pipelineInfo.vertPath);
    auto fragShaderCode = vkTools::readFile(pipelineInfo.fragPath);

    spv_reflect::ShaderModule vertShaderModuleInfo(vertShaderCode.size(), vertShaderCode.data());
    spv_reflect::ShaderModule fragShaderModuleInfo(fragShaderCode.size(), fragShaderCode.data());

    auto vertShaderModule = createShaderModule(vertShaderCode);
    auto fragShaderModule = createShaderModule(fragShaderCode);

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        {
            .flags = vk::PipelineShaderStageCreateFlags(),
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = static_cast<VkShaderModule>(vertShaderModule),
            .pName = vertShaderModuleInfo.GetEntryPointName()
        },
        {
            .flags = vk::PipelineShaderStageCreateFlags(),
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = static_cast<VkShaderModule>(fragShaderModule),
            .pName = fragShaderModuleInfo.GetEntryPointName()
        }
    };


    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE,
    };

    vk::Extent2D extent = m_renderPasses[pipelineInfo.renderPassId]->getRenderPassExtent();

    vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)extent.width,
        .height = (float)extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissor = {
        .offset = { 0, 0 },
        .extent = extent,
    };

    vk::PipelineViewportStateCreateInfo viewportState = {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer = {
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = pipelineInfo.polygonMode,
        .cullMode = pipelineInfo.cullmode,
        .frontFace = pipelineInfo.frontFace,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = pipelineInfo.lineWidth,
    };

    vk::PipelineMultisampleStateCreateInfo multisampling = {
        .rasterizationSamples = pipelineInfo.isMultisampled ? m_msaaSampleCount : vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = pipelineInfo.isMultisampled ? VK_TRUE : VK_FALSE,
        .minSampleShading = 1.f,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending = {
        .logicOpEnable = VK_FALSE,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;



    vk::PipelineDepthStencilStateCreateInfo depthStencilState{
        .depthTestEnable = pipelineInfo.depthTestEnable,
        .depthWriteEnable = pipelineInfo.depthWriteEnable,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };


    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlending,
        .layout = m_pipelineLayout,
        .renderPass = m_renderPasses[pipelineInfo.renderPassId]->getRenderPass(),
        .subpass = 0,
        .basePipelineHandle = nullptr,
    };

    auto pipelineResult = m_device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo);

    if (pipelineResult.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("could not create pipeline");
    }
    

    m_device.destroyShaderModule(vertShaderModule);
    m_device.destroyShaderModule(fragShaderModule);
    
    vk::Pipeline pipeline = pipelineResult.value;
    m_context->setDebugObjectName((uint64_t)static_cast<VkPipeline>(pipeline), static_cast<VkDebugReportObjectTypeEXT>(pipeline.debugReportObjectType), "PARCE QUE C'EST NOTRE PIPELINE");


    return VulkanPipeline {
        .pipelineInfo = pipelineInfo,
        .pipeline = pipeline
    };
}


#pragma endregion

#pragma region SHADER_MODULES
//Returns a shader module from spir-v shader code string
vk::ShaderModule VulkanRenderer::createShaderModule(std::vector<char>& shaderCode) 
{
    try {
        vk::ShaderModuleCreateInfo createInfo{
            .flags = vk::ShaderModuleCreateFlags(),
            .codeSize = shaderCode.size(),
            .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())
        };
        return m_device.createShaderModule(createInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create shader module!");
    }
}

#pragma endregion

#pragma region DESCRIPTORS
//creates the main descriptor set layout
void VulkanRenderer::createDescriptorSetLayout()
{
    //Set 0: Transforms Uniform Buffer, Texture sampler
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
    };

    vk::DescriptorSetLayoutBinding samplerLayoutBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = MAX_TEXTURE_COUNT,  //Dynamic Indexing
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutBinding bindings[2] = { uboLayoutBinding, samplerLayoutBinding};
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
        m_mainDescriptorSetLayout = m_device.createDescriptorSetLayout(layoutInfo);
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
        m_shadowDescriptorSetLayout = m_device.createDescriptorSetLayout(shadowLayoutInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor set layout");
    }
}

//Creates the main descriptor pool
void VulkanRenderer::createDescriptorPools() {

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
        m_mainDescriptorPool = m_device.createDescriptorPool(poolInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor pool");
    }


    vk::DescriptorPoolSize shadowPoolSize;
    shadowPoolSize.type = vk::DescriptorType::eCombinedImageSampler;//Dynamic Indexing
    shadowPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    vk::DescriptorPoolCreateInfo shadowPoolInfo{
        .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .poolSizeCount = 1,
        .pPoolSizes = &shadowPoolSize,
    };

    try {
        m_shadowDescriptorPool = m_device.createDescriptorPool(shadowPoolInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor pool");
    }
}

void VulkanRenderer::createShadowDescriptorSets() {
    vk::DescriptorImageInfo shadowTextureImageInfo{
        .sampler = m_textureSampler,
        .imageView = reinterpret_cast<ShadowRenderPass*>(m_renderPasses[RenderPassesId::ShadowMappingPassId])->getShadowAttachment(),
        .imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal,
    };

    m_shadowDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    std::vector<vk::DescriptorSetLayout> shadowLayouts(MAX_FRAMES_IN_FLIGHT, m_shadowDescriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocInfo = {
         .descriptorPool = m_shadowDescriptorPool,
         .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
         .pSetLayouts = shadowLayouts.data()
    };

    try {
        m_shadowDescriptorSets = m_device.allocateDescriptorSets(allocInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("could not allocate descriptor sets");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::WriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.dstSet = m_shadowDescriptorSets[i];
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescriptorSet.pImageInfo = &shadowTextureImageInfo;

        try {
            m_device.updateDescriptorSets(writeDescriptorSet, nullptr);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor sets");
        }
    }

}

//creates and populates the scene's descriptors
std::vector<vk::DescriptorSet> VulkanRenderer::createDescriptorSets(VulkanScene* scene) 
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
                
            }else
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
            if (texturedMesh.normalMapImage != nullptr && !texturedMesh.textureImage->hasLoadingFailed())
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

    std::vector<vk::DescriptorSet> descriptorSets;
    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
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
        descriptorSets = m_device.allocateDescriptorSets(allocInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("could not allocate descriptor sets");
    }

    //Updating the descriptor sets with the appropriates references
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = m_uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };      

        std::vector<vk::WriteDescriptorSet> descriptorWrites(2);
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorCount = textureImageInfo.size();
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].pImageInfo = textureImageInfo.data();


        try {
            m_device.updateDescriptorSets(descriptorWrites, nullptr);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor sets");
        }
    }
    return descriptorSets;
}

//Creates Uniform buffers, samplers or other shader readable objects
void VulkanRenderer::createDescriptorObjects() {
    createDefaultTextures();
    createUniformBuffer();
    createTextureSampler();
}

//Creates the uniform buffer needed for View and Projection matrices
void VulkanRenderer::createUniformBuffer() {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::tie(m_uniformBuffers[i], m_uniformBuffersAllocations[i]) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);
    }
}

//Creates the main texture sampler
void VulkanRenderer::createTextureSampler() {
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
        m_textureSampler = m_device.createSampler(samplerInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("failed to create texture sampler");
    }

}

//Creates a default VulkanImage to be used a default texture
void VulkanRenderer::createDefaultTextures() 
{
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
#pragma endregion

#pragma region PUSH_CONSTANTS
//Creates the push constant range for pipeline creation
void VulkanRenderer::createPushConstantRanges()
{
    m_pushConstantRanges.resize(PUSH_CONSTANTS_COUNT);
    vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = 128,
    };

    m_pushConstantRanges[0] = pushConstantRange;
}
#pragma endregion

#pragma region FRAMEBUFFERS


//Recreate objects that depend on the swapchain image size to handle window resizing
void VulkanRenderer::recreateSwapchainSizedObjects() {
    cleanSwapchainSizedObjects();
    m_context->recreateSwapchain();
    for (const auto& renderPass : m_renderPasses)
    {
        renderPass->recreateRenderPass();
    }

    createPipelineLayout();
    for (int i = 0; i < m_pipelines.size(); i++)
    {
        m_pipelines[i] = createPipeline(m_pipelines[i].pipelineInfo);
    }
    m_shadowPipeline = createPipeline(m_shadowPipeline.pipelineInfo);
    
}

//Destroys  objects that depend on the swapchain image size to handle window resizing
void VulkanRenderer::cleanSwapchainSizedObjects() {
    vkDeviceWaitIdle(m_device);

    for (const auto& pipeline : m_pipelines) {
        m_device.destroyPipeline(pipeline.pipeline);
    }
    m_device.destroyPipeline(m_shadowPipeline.pipeline);
    m_device.destroyPipelineLayout(m_pipelineLayout);
    
    m_context->cleanupSwapchain();

}

void VulkanRenderer::createFramebuffers() {
    for (auto& renderPass : m_renderPasses) {
        renderPass->createAttachments();
        renderPass->createFramebuffer();
    }
}

#pragma endregion

#pragma region COMMAND_BUFFERS
//creates the main command buffers
void VulkanRenderer::createCommandBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_context->getCommandPool(),
        .level = vk::CommandBufferLevel::ePrimary, //Primary : Can be submitted to a queue for execution, Secondary : Can be called from primary command buffers
        .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size()),
    };

    try {
        m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not allocate command buffers");
    }
}

//Records the main command buffer for frame generation
void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex) 
{

    vk::CommandBufferBeginInfo beginInfo{};
    commandBuffer.begin(beginInfo);
    m_renderPasses[0]->drawRenderPass(commandBuffer, swapchainImageIndex, m_currentFrame, m_shadowDescriptorSets[m_currentFrame], m_shadowPipeline.pipeline, m_scenes, m_pipelineLayout);
    m_renderPasses[1]->drawRenderPass(commandBuffer, swapchainImageIndex, m_currentFrame, m_shadowDescriptorSets[m_currentFrame], m_pipelines[m_currentPipelineId].pipeline, m_scenes, m_pipelineLayout);
    commandBuffer.end();
}
#pragma endregion
     

#pragma region SYNCHRONISATION
//create Synchronisation objects to manage frame generation execution
void VulkanRenderer::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreInfo{};

    vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};//Signaled by default so that the mainloop doesn't block at the first frame

    try {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            m_imageAvailableSemaphores[i] =  m_device.createSemaphore(semaphoreInfo);
            m_renderFinishedSemaphores[i] = m_device.createSemaphore(semaphoreInfo);
            m_inFlightFences[i] = m_device.createFence(fenceInfo);
        }
    }catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create synchronisation objects");
    }

}
#pragma endregion

#pragma region EXECUTION_FLOW
//Starts the window management and frame drawing loop
void VulkanRenderer::mainloop() {
    while (m_context->isWindowOpen())
    {
        m_context->manageWindow();
        manageInput();
        updateEntities();
        drawFrame();
    }
    m_device.waitIdle();
}

//Draws and presents a frame when a swapchain image is available
void VulkanRenderer::drawFrame() {

    //Wait and reset CPU semaphore
    m_device.waitForFences(m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);//Wait for one or all fences (VK_TRUE), uint64_max disables the timeout
    uint32_t imageIndex = m_context->acquireNextSwapchainImage(m_imageAvailableSemaphores[m_currentFrame]);
    m_device.resetFences(m_inFlightFences[m_currentFrame]); //Always reset fences (after being sure we are going to submit work
    m_commandBuffers[m_currentFrame].reset(); //Reset to record the command buffer
    

    updateUniformBuffers(m_currentFrame); //TODO abstract application dependent code.
    recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);
  
    //Submitting the command buffer
    vk::Semaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1, //Specifies which semaphores to wait on before executions begin and on which stage to wait.
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages, //Vertex shader can be ran while the image is not yet available. (Stages are linked to waitSemaphores indexes)
        .commandBufferCount = 1, //Command buffers to submit for execution
        .pCommandBuffers = &m_commandBuffers[m_currentFrame],
        .signalSemaphoreCount = 1,  //Semaphores to signal once the buffers have finished execution
        .pSignalSemaphores = signalSemaphores,
    };

    //inFlightFence will be signaled when the command buffers finished executing
    try {
        m_context->getGraphicsQueue().submit(submitInfo, m_inFlightFences[m_currentFrame]);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }


    if (!present(signalSemaphores, imageIndex)) {
        recreateSwapchainSizedObjects();
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

/// <summary>
/// Presents the swapchain image of index imageIndex
/// </summary>
/// <param name="signalSemaphores"></param>
/// <param name="imageIndex"></param>
/// <returns>false if the swapchain is no longer valid. It means the swapchain and swapchain size related objects have to be recreated</returns>
bool VulkanRenderer::present(vk::Semaphore *signalSemaphores, uint32_t imageIndex) {

    vk::SwapchainKHR swapchains[] = { m_context->getSwapchain()};
    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };
    
    vk::Result result = m_context->getPresentQueue().presentKHR(presentInfo);
    
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_context->isFramebufferResized())
    {
        m_context->clearFramebufferResized();
        return false;
    }
    else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    return true;
}

//Updates the view, model matrices in the uniform buffer. This is done outside of command buffer recording
void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex) {
    //Time since rendering start
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - startTime).count();
    vk::Extent2D extent = m_renderPasses[RenderPassesId::MainRenderPassId]->getRenderPassExtent();

    float speed = 0.1f;
    //Model View Proj
    UniformBufferObject ubo{};
    ubo.view = m_camera->getViewMatrix();
    ubo.proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 1000.0f); //45deg vertical field of view, aspect ratio, near and far view planes
    ubo.proj[1][1] *= -1; //Designed for openGL but the Y coordinate of the clip coordinates is inverted
  
    ubo.lightView = glm::lookAt(glm::vec3(0.f, 17.f, 4.f * sin(time)), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    ubo.lightProj = glm::perspective(glm::radians(45.0f), 1.f, 1.f, 40.0f);
    ubo.lightProj[1][1] *= -1;
    


    void* data = m_allocator.mapMemory(m_uniformBuffersAllocations[imageIndex]);
    memcpy(data, &ubo, sizeof(ubo));
    m_allocator.unmapMemory(m_uniformBuffersAllocations[imageIndex]);
}

#pragma endregion

#pragma region SCENES
//Adds a scene to the scenes to be renderer. Generates the descriptor sets with the textures before adding.
void VulkanRenderer::addScene(VulkanScene* vulkanScene) {
    vulkanScene->createBuffers();
    vulkanScene->setDescriptorSets(createDescriptorSets(vulkanScene));
    m_scenes.push_back(vulkanScene);
}
#pragma endregion

#pragma region RENDER_PASSES
void VulkanRenderer::createRenderPasses() {
    //Render pass 1: Shadow Render Pass
    ShadowRenderPass* shadowRenderPass = new ShadowRenderPass(m_context); 
    shadowRenderPass->createRenderPass();
    m_renderPasses.push_back(shadowRenderPass);

    //Render pass 2: Main Render Pass
    MainRenderPass* mainRenderPass = new MainRenderPass(m_context, shadowRenderPass);
    mainRenderPass->createRenderPass();
    m_renderPasses.push_back(mainRenderPass);

}
#pragma endregion RENDER_PASSES

#pragma region INPUT
void VulkanRenderer::manageInput() {
    static bool rightPressed = false;
    static bool leftPressed = false;

    GLFWwindow* window = m_context->getWindowPtr();
    int key = glfwGetKey(window, GLFW_KEY_RIGHT);
    if (key == GLFW_PRESS && rightPressed == false) {
        m_currentPipelineId++;
        rightPressed = true;
    }
    else if (rightPressed == true && key == GLFW_RELEASE)
    {
        rightPressed = false;
    }

    key = glfwGetKey(window, GLFW_KEY_LEFT);
    if (key == GLFW_PRESS && leftPressed == false) {
        m_currentPipelineId--;
        leftPressed = true;
    }
    else if (leftPressed == true && key == GLFW_RELEASE)
    {
        leftPressed = false;
    }
    if (m_currentPipelineId == -1) m_currentPipelineId = static_cast<uint32_t>(m_pipelines.size()) - 1;
    m_currentPipelineId = m_currentPipelineId % static_cast<uint32_t>(m_pipelines.size());
}

#pragma endregion


#pragma region ENTITIES
void VulkanRenderer::updateEntities() {
    for (auto& entity : m_entities) {
        entity->update();
    }
}

void VulkanRenderer::registerEntity(Entity* entity) {
    m_entities.push_back(entity);
}
#pragma endregion ENTITIES