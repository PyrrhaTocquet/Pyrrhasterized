/*
author: Pyrrha Tocquet
date: 07/06/23
desc: Render pass that renders the multiple shadow cascades
*/

#pragma once
#include "DepthOnlyPass.h"
#include "DirectionalLight.h"
#include "Camera.h"

class ShadowCascadeRenderPass : public DepthOnlyPass 
{
private:
	std::vector<vk::ImageView>	m_shadowDepthLayerViews;
	DirectionalLight*			m_sun;

	const float					c_constantDepthBias = 3.0f;
	const float					c_slopeScaleDepthBias = 15.0f;
	const uint32_t				c_shadowMapSize = 4096u;
public:
	float m_cascadeSplitLambda = 0.95f;
	float m_shadowMapsBlendWidth = 0.5f;
	ShadowCascadeRenderPass(VulkanContext* context, vk::DescriptorSetLayout geometryDescriptorSetLayout);
	virtual ~ShadowCascadeRenderPass()override;
	void createFramebuffer()override;
	void createAttachments()override;
	void cleanAttachments()override;
	void createRenderPass()override;
	void createDefaultPipeline()override;
	void createPipelineRessources()override;
	void createPushConstantsRanges()override;
	void updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes)override;
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t currentFrame, std::vector<VulkanScene*> scenes)override;
	void recreateRenderPass() override;
	vk::Extent2D getRenderPassExtent() override;


private:
	void recordShadowCascadeMemoryDependency(vk::CommandBuffer commandBuffer);
	vk::DescriptorBufferInfo getUboInfo(VulkanScene *scene, const uint32_t frame);
};