#include "ShadowCascadeRenderPass.h"

ShadowCascadeRenderPass::ShadowCascadeRenderPass(VulkanContext* context, Camera* camera)
{
    m_context = context;
    m_camera = camera;
    std::vector<vk::ImageView> swapchainImageViews = m_context->getSwapchainImageViews();
    //One per cascade times two per frames frames in flight
    m_framebuffers.resize(m_context->getSwapchainImagesCount() * SHADOW_CASCADE_COUNT);
    m_shadowDepthLayerViews.resize(m_context->getSwapchainImagesCount() * SHADOW_CASCADE_COUNT);

}

ShadowCascadeRenderPass::~ShadowCascadeRenderPass() {
    cleanAttachments();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_context->getAllocator().destroyBuffer(m_uniformBuffers[i], m_uniformBuffersAllocations[i]);
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
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
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

void ShadowCascadeRenderPass::createDescriptorSet(VulkanScene* scene)
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
            .buffer = m_uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
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

void ShadowCascadeRenderPass::createPipelineLayout()
{

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
       .setLayoutCount = 1,
       .pSetLayouts = &m_mainDescriptorSetLayout,
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
        .vertPath = "shaders/vertexCSM.spv",
        .fragPath = "shaders/fragmentCSM.spv",
        .cullmode = vk::CullModeFlagBits::eNone,
        .renderPassId = RenderPassesId::ShadowMappingPassId,
        .isMultisampled = false,
    };

    m_mainPipeline = new VulkanPipeline(m_context, pipelineInfo, m_pipelineLayout, m_renderPass, getRenderPassExtent());

}

void ShadowCascadeRenderPass::createPipelineRessources()
{
    createUniformBuffer();
}

void ShadowCascadeRenderPass::createPushConstantsRanges()
{
    m_pushConstant = vk::PushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = 128,
    };
}

void ShadowCascadeRenderPass::updatePipelineRessources(uint32_t currentFrame)
{
    updateUniformBuffer(currentFrame);
}


void ShadowCascadeRenderPass::drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t currentFrame, std::vector<VulkanScene*> scenes)
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
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_mainPipeline->getPipeline());
        VkDeviceSize offset = 0;
        //Draws each scene
        for (auto& scene : scenes)
        {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_mainDescriptorSet[currentFrame], nullptr);
           
            scene->draw(commandBuffer, currentFrame, m_pipelineLayout);
        }

        commandBuffer.endRenderPass();
    }
   

}

void ShadowCascadeRenderPass::createUniformBuffer()
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::tie(m_uniformBuffers[i], m_uniformBuffersAllocations[i]) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);
    }
}

void ShadowCascadeRenderPass::updateUniformBuffer(uint32_t currentFrame)
{
    //Time since rendering start
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - startTime).count();
    vk::Extent2D extent = getRenderPassExtent();

    float speed = 0.1f;


    //Model View Proj
    UniformBufferObject ubo{};
    ubo.view = m_camera->getViewMatrix();
    ubo.proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 150.0f); //45deg vertical field of view, aspect ratio, near and far view planes
    ubo.proj[1][1] *= -1; //Designed for openGL but the Y coordinate of the clip coordinates is inverted

    glm::vec3 lightPos = glm::vec3(0.f, 17.f, 4.f * sin(time));

    ubo.lightView = glm::lookAt(lightPos, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    ubo.lightProj = glm::perspective(glm::radians(45.0f), 1.f, 1.f, 40.0f);
    ubo.lightProj[1][1] *= -1;
    /*
    //TODO Extract update cascades
    float nearClip = 0.1f;
    float farClip = 150.f;
    float clipRange = farClip - nearClip;

    float minZ = nearClip;
    float maxZ = nearClip + clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32_t i = 0; i < 4; i++) {
        float p = (i + 1) / static_cast<float>(4);
        float log = minZ * std::pow(ratio, p);
        float uniform = minZ + range * p;
        float d = 0.95f * (log - uniform) + uniform;
        ubo.cascadeSplits[i] = (d - nearClip);
    }

    float lastSplitDist = 0.0;
    for (uint32_t i = 0; i < 4; i++) {
        float splitDist = ubo.cascadeSplits[i] / clipRange;

        glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f,  1.0f, 0.0f),
            glm::vec3(1.0f,  1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f,  1.0f,  1.0f),
            glm::vec3(1.0f,  1.0f,  1.0f),
            glm::vec3(1.0f, -1.0f,  1.0f),
            glm::vec3(-1.0f, -1.0f,  1.0f),
        };

        // Project frustum corners into world space
        glm::mat4 invCam = glm::inverse(ubo.proj * ubo.view);
        for (uint32_t i = 0; i < 8; i++) {
            glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
            frustumCorners[i] = invCorner / invCorner.w;
        }

        for (uint32_t i = 0; i < 4; i++) {
            glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
            frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
            frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
        }

        // Get frustum center
        glm::vec3 frustumCenter = glm::vec3(0.0f);
        for (uint32_t i = 0; i < 8; i++) {
            frustumCenter += frustumCorners[i];
        }
        frustumCenter /= 8.0f;

        float radius = 0.0f;
        for (uint32_t i = 0; i < 8; i++) {
            float distance = glm::length(frustumCorners[i] - frustumCenter);
            radius = glm::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3(radius);
        glm::vec3 minExtents = -maxExtents;

        glm::vec3 lightDir = normalize(-lightPos);
        glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

        // Store split distance and matrix in cascade //TODONOW
        /*cascades[i].splitDepth = (nearClip + splitDist * clipRange) * -1.0f;
        cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;
        //TODO C'est dégueulasse
        lastSplitDist = ubo.cascadeSplits[i];*
    }*/

    void* data = m_context->getAllocator().mapMemory(m_uniformBuffersAllocations[currentFrame]);
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    m_context->getAllocator().unmapMemory(m_uniformBuffersAllocations[currentFrame]);
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
