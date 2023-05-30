#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "ShadowRenderPass.h"


class ShadowRenderPass;

class MainRenderPass : public VulkanRenderPass {
	//created
	VulkanImage* m_colorAttachment = nullptr;
	VulkanImage* m_depthAttachment = nullptr;

	//acquired at construction
	ShadowRenderPass* m_shadowRenderPass = nullptr;
public:
	MainRenderPass(VulkanContext* context, ShadowRenderPass* shadowRenderPass);
	~MainRenderPass();
	void createRenderPass() override;
	void createFramebuffer() override;
	void createAttachments() override;
	void cleanAttachments() override;
	void recreateRenderPass() override;
	vk::Extent2D getRenderPassExtent() override;
	void renderImGui(vk::CommandBuffer commandBuffer);
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, vk::DescriptorSet descriptorSet, vk::Pipeline pipeline, std::vector<VulkanScene*> scenes, vk::PipelineLayout pipelineLayout) override;

};