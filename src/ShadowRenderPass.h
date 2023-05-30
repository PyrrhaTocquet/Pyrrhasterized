#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanRenderPass.h"
#include "VulkanRenderer.h"

class ShadowRenderPass : public VulkanRenderPass {

	const uint32_t SHADOW_MAP_SIZE = 4096;

private:
	VulkanImage* m_shadowDepthAttachment = nullptr;
public:
	ShadowRenderPass(VulkanContext* context);
	~ShadowRenderPass() override;
	void createRenderPass() override;
	void createFramebuffer() override;
	void createAttachments() override;
	void cleanAttachments() override;
	void recreateRenderPass() override;
	void drawRenderPass() override {};
	vk::Extent2D getRenderPassExtent() override;
	[[nodiscard]] vk::ImageView getShadowAttachment();

};