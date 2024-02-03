#pragma once
#include "Defs.h"
#include "VulkanContext.h"
#include "spirv_reflect.h"
#include "VulkanTools.h"
#include <optional>

struct PipelineInfo {
	std::optional<const char*> taskShaderPath = std::nullopt;
	const char* meshShaderPath;
	const char* fragShaderPath;
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlags cullmode = vk::CullModeFlagBits::eBack;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	float lineWidth = 1.0f;
	vk::Bool32 depthTestEnable = VK_TRUE;
	vk::Bool32 depthWriteEnable = VK_TRUE;
	RenderPassesId renderPassId = RenderPassesId::MainRenderPassId;
	bool isMultisampled = true;
	float depthBias[2] = { 0.f, 0.f }; //[0] is constant facto [1] is slope factor
};

class VulkanPipeline {
private:
	PipelineInfo m_pipelineInfo;
	vk::PipelineLayout m_pipelineLayout;
	vk::RenderPass m_renderPass;
	vk::Pipeline m_pipeline;
	VulkanContext* m_context;

public:
	
	VulkanPipeline(VulkanContext* context, PipelineInfo pipelineInfo, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, vk::Extent2D extent);
	~VulkanPipeline();
	vk::ShaderModule createShaderModule(std::vector<char>& shaderCode);
	void cleanPipeline();
	void recreatePipeline(vk::Extent2D extent);
	vk::Pipeline getPipeline();
};