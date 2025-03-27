#include "VulkanPipeline.h"


VulkanPipeline::VulkanPipeline(VulkanContext* context, PipelineInfo pipelineInfo, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, vk::Extent2D extent)
{
    m_pipelineInfo = pipelineInfo;
    m_pipelineLayout = pipelineLayout;
    m_renderPass = renderPass;
    m_context = context;

    recreatePipeline(extent);

}

VulkanPipeline::~VulkanPipeline()
{
    m_context->getDevice().destroyPipeline(m_pipeline);
}


vk::ShaderModule VulkanPipeline::createShaderModule( std::vector<char>& shaderCode)
{
    try {
        vk::ShaderModuleCreateInfo createInfo{
            .flags = vk::ShaderModuleCreateFlags(),
            .codeSize = shaderCode.size(),
            .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())
        };
        return m_context->getDevice().createShaderModule(createInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create shader module!");
    }
}

void VulkanPipeline::cleanPipeline()
{
    m_context->getDevice().destroyPipeline(m_pipeline);
}

void VulkanPipeline::recreatePipeline(vk::Extent2D extent)
{
    vk::Device device = m_context->getDevice();
    auto meshShaderCode = vkTools::readFile(m_pipelineInfo.meshShaderPath);
    auto fragShaderCode = vkTools::readFile(m_pipelineInfo.fragShaderPath);

    spv_reflect::ShaderModule meshShaderModuleInfo(meshShaderCode.size(), meshShaderCode.data());
    spv_reflect::ShaderModule fragShaderModuleInfo(fragShaderCode.size(), fragShaderCode.data());

    auto meshShaderModule = createShaderModule(meshShaderCode);
    auto fragShaderModule = createShaderModule(fragShaderCode);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
        {
            .flags = vk::PipelineShaderStageCreateFlags(),
            .stage = vk::ShaderStageFlagBits::eMeshEXT,
            .module = static_cast<VkShaderModule>(meshShaderModule),
            .pName = meshShaderModuleInfo.GetEntryPointName()
        },
        {
            .flags = vk::PipelineShaderStageCreateFlags(),
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = static_cast<VkShaderModule>(fragShaderModule),
            .pName = fragShaderModuleInfo.GetEntryPointName()
        }
    };

    if(m_pipelineInfo.taskShaderPath.has_value())
    {
        auto taskShaderCode = vkTools::readFile(m_pipelineInfo.taskShaderPath.value());
        spv_reflect::ShaderModule taskShaderModuleInfo(taskShaderCode.size(), taskShaderCode.data());
        auto taskShaderModule = createShaderModule(taskShaderCode);

        vk::PipelineShaderStageCreateInfo taskStageCreateInfo{
            .flags = vk::PipelineShaderStageCreateFlags(),
            .stage = vk::ShaderStageFlagBits::eTaskEXT,
            .module = static_cast<VkShaderModule>(taskShaderModule),
            .pName = "main"
        };
        shaderStages.insert(shaderStages.begin(), taskStageCreateInfo);
        device.destroyShaderModule(taskShaderModule);
    }


    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    /*vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };*/

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE,
    };


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
        .polygonMode = m_pipelineInfo.polygonMode,
        .cullMode = m_pipelineInfo.cullmode,
        .frontFace = m_pipelineInfo.frontFace,
        .depthBiasEnable = VK_TRUE,
        .depthBiasConstantFactor = m_pipelineInfo.depthBias[0],
        .depthBiasSlopeFactor = m_pipelineInfo.depthBias[1],
        .lineWidth = m_pipelineInfo.lineWidth,
    };

    vk::PipelineMultisampleStateCreateInfo multisampling = {
        .rasterizationSamples = m_pipelineInfo.isMultisampled ? m_context->getMaxUsableSampleCount() : vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = m_pipelineInfo.isMultisampled ? VK_TRUE : VK_FALSE,
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
        .depthTestEnable = m_pipelineInfo.depthTestEnable,
        .depthWriteEnable = m_pipelineInfo.depthWriteEnable,
        .depthCompareOp = vk::CompareOp::eLessOrEqual,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };


    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        /*.pVertexInputState = &vertexInputInfo,*/
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlending,
        .layout = m_pipelineLayout,
        .renderPass = m_renderPass,
        .subpass = 0,
        .basePipelineHandle = nullptr,
    };

    auto pipelineResult = device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo);

    if (pipelineResult.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("could not create pipeline");
    }

    device.destroyShaderModule(meshShaderModule);
    device.destroyShaderModule(fragShaderModule);    

    m_pipeline = pipelineResult.value;
    m_context->setDebugObjectName((uint64_t)static_cast<VkPipeline>(m_pipeline), static_cast<VkDebugReportObjectTypeEXT>(m_pipeline.debugReportObjectType), "PARCE QUE C'EST NOTRE PIPELINE");
}

vk::Pipeline VulkanPipeline::getPipeline()
{
    return m_pipeline;
}
