#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanRenderPass.h"


class ShadowRenderPass : public VulkanRenderPass {

	const uint32_t SHADOW_MAP_SIZE = 4096;

protected:
	VulkanImage* m_shadowDepthAttachment = nullptr;
public:
	ShadowRenderPass(VulkanContext* context);
	ShadowRenderPass() {};
	virtual ~ShadowRenderPass() override;
	void createRenderPass() override;
	virtual void createFramebuffer() override;
	virtual void createAttachments() override;
	virtual void cleanAttachments() override;
	void recreateRenderPass() override;
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, vk::DescriptorSet descriptorSet, vk::Pipeline pipeline, std::vector<VulkanScene*> scenes, vk::PipelineLayout pipelineLayout) override;
	vk::Extent2D getRenderPassExtent() override;
	[[nodiscard]] vk::ImageView getShadowAttachment();

};