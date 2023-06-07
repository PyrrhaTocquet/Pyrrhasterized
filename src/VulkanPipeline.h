#pragma once
#include "Defs.h"
#include "VulkanRenderPass.h"
#include "VulkanContext.h"
#include "spirv_reflect.h"

struct PipelineInfo {
	const char* vertPath;
	const char* fragPath;
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlags cullmode = vk::CullModeFlagBits::eBack;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	float lineWidth = 1.0f;
	vk::Bool32 depthTestEnable = VK_TRUE;
	vk::Bool32 depthWriteEnable = VK_TRUE;
	RenderPassesId renderPassId = RenderPassesId::MainRenderPassId;
	bool isMultisampled = true;
};

class VulkanPipeline {
private:
	PipelineInfo m_pipelineInfo;
	vk::PipelineLayout m_pipelineLayout;
	VulkanRenderPass* m_renderPass;
	vk::Pipeline m_pipeline;
	VulkanContext* m_context;

public:
	
	VulkanPipeline(VulkanContext* context, PipelineInfo pipelineInfo, vk::PipelineLayout pipelineLayout, VulkanRenderPass* renderPass);
	~VulkanPipeline();
	vk::ShaderModule createShaderModule(std::vector<char>& shaderCode);
	void cleanPipeline();
	void recreatePipeline();
	vk::Pipeline getPipeline();
};