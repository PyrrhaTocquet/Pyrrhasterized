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
	void drawRenderPass() override {};

};