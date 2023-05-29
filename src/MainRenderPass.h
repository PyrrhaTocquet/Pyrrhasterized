#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanRenderPass.h"
#include "ShadowRenderPass.h"
#include "VulkanRenderer.h"

class ShadowRenderPass;

class MainRenderPass : public VulkanRenderPass {
	//created
	VulkanImage* m_colorAttachment = nullptr;
	VulkanImage* m_depthAttachment = nullptr;

	//acquired at construction
	vk::ImageView m_shadowDepthImageView = VK_NULL_HANDLE;
public:
	MainRenderPass(VulkanContext* context, ShadowRenderPass* shadowRenderPass);
	void createRenderPass() override;
	~MainRenderPass() override;
	void createFramebuffer() override;
	void recreateRenderPass() override;
	void createAttachments() override;
	void cleanAttachments() override;
	vk::Extent2D getRenderPassExtent() override;

};