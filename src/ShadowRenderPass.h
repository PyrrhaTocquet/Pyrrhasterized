#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanRenderPass.h"


class ShadowRenderPass : public VulkanRenderPass {

	const uint32_t SHADOW_MAP_SIZE = 4096;

protected:
	VulkanImage* m_shadowDepthAttachment;
public:
	ShadowRenderPass(VulkanContext* context);
	ShadowRenderPass() {};
	virtual ~ShadowRenderPass() override;
	void createRenderPass() override;
	virtual void createFramebuffer() override;
	virtual void createAttachments() override;
	virtual void cleanAttachments() override;
	virtual void createDescriptorPool()override {}; //TODO
	virtual void createDescriptorSetLayout()override {};
	virtual void createDescriptorSet(VulkanScene* scene)override {};
	virtual void createPipelineLayout()override {};
	virtual void createDefaultPipeline()override {};
	virtual void recreateRenderPass() override;
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) override {};
	vk::Extent2D getRenderPassExtent() override;
	[[nodiscard]] vk::ImageView getShadowAttachment();

};