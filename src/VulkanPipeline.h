#pragma once
#include "Defs.h"
#include "VulkanContext.h"
#include "spirv_reflect.h"
#include "VulkanTools.h"
#include <optional>

struct RenderPipelineInfo {
	std::optional<const char*> taskShaderPath = std::nullopt;
	const char* meshShaderPath{};
	const char* fragShaderPath{};
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlags cullmode = vk::CullModeFlagBits::eBack;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	float lineWidth = 1.0f;
	vk::Bool32 depthTestEnable = VK_TRUE;
	vk::Bool32 depthWriteEnable = VK_TRUE;
	PassesId renderPassId = PassesId::MainRenderPassId;
	bool isMultisampled = true;
	float depthBias[2] = { 0.f, 0.f }; //[0] is constant facto [1] is slope factor
};

struct ComputePipelineInfo
{
	const char* computePath;
	PassesId computePassId;
};

class VulkanPipeline
{
protected:
	VulkanContext* m_context = nullptr;
	vk::Pipeline m_pipeline{};
	vk::PipelineLayout m_pipelineLayout{};
public:
	VulkanPipeline() = default;
	VulkanPipeline(VulkanContext *context, vk::PipelineLayout layout) : m_context(context), m_pipelineLayout(layout) {};
	virtual void cleanPipeline();
	virtual void recreatePipeline(vk::Extent2D extent) = 0;
	virtual vk::Pipeline getPipeline() { return m_pipeline; };
	virtual vk::ShaderModule createShaderModule(std::vector<char>& shaderCode);
};

class VulkanComputePipeline : public VulkanPipeline
{
	VulkanContext *m_context = nullptr;
	ComputePipelineInfo m_pipelineInfo{};
	
	VulkanComputePipeline(VulkanContext *context, ComputePipelineInfo pipelineInfo, vk::PipelineLayout pipelineLayout, vk::Extent2D extent);
	~VulkanComputePipeline();
	virtual void recreatePipeline(vk::Extent2D extent);
};


class VulkanRenderPipeline : public VulkanPipeline {
private:
	RenderPipelineInfo m_pipelineInfo{};
	vk::RenderPass m_renderPass{};
public:
	VulkanRenderPipeline() = default;
	VulkanRenderPipeline(VulkanContext* context, RenderPipelineInfo pipelineInfo, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, vk::Extent2D extent);
	~VulkanRenderPipeline();
	virtual void recreatePipeline(vk::Extent2D extent) override;
};