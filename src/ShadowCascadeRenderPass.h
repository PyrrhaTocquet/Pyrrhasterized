/*
author: Pyrrha Tocquet
date: 07/06/23
desc: Render pass that renders the multiple shadow cascades
*/

#pragma once
#include "ShadowRenderPass.h"
#include "DirectionalLight.h"
#include "Camera.h"

struct CascadeUniformObject {
	glm::mat4 cascadeViewProjMat[4];
	float cascadeSplits[4];
};

class ShadowCascadeRenderPass : public ShadowRenderPass {
private:
	std::vector<vk::ImageView> m_shadowDepthLayerViews;

	Camera* m_camera;
	DirectionalLight* m_sun;
	std::vector<vk::Buffer> m_uniformBuffers;
	std::vector<vma::Allocation> m_uniformBuffersAllocations;

	std::array<CascadeUniformObject, MAX_FRAMES_IN_FLIGHT> m_cascadeUbos;


	const float c_constantDepthBias = 3.0f;
	const float c_slopeScaleDepthBias = 15.0f;
public:
	float m_cascadeSplitLambda = 0.95f;
	float m_shadowMapsBlendWidth = 0.5f;
	ShadowCascadeRenderPass(VulkanContext* context, Camera* camera);
	virtual ~ShadowCascadeRenderPass()override;
	void createFramebuffer()override;
	void createAttachments()override;
	void cleanAttachments()override;
	void createDescriptorPool()override;
	void createDescriptorSetLayout()override;
	void createDescriptorSets(VulkanScene* scene)override;
	void createPipelineLayout()override;
	void createDefaultPipeline()override;
	void createPipelineRessources()override;
	void createPushConstantsRanges()override;
	void updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*> scenes)override;
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t currentFrame, std::vector<VulkanScene*> scenes)override;
	void recreateRenderPass() override;
	CascadeUniformObject getCurrentUbo(uint32_t currentFrame);

private:
	void createUniformBuffer();
	void updateUniformBuffer(uint32_t currentFrame);


	void recordShadowCascadeMemoryDependency(vk::CommandBuffer commandBuffer);
};