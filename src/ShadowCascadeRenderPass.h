#pragma once
#include "ShadowRenderPass.h"
#include "Camera.h"

struct CascadeUniformObject {
	float cascadeSplits[4];
	glm::mat4 cascadeViewProjMat[4];
};

class ShadowCascadeRenderPass : public ShadowRenderPass {
private:
	std::vector<vk::ImageView> m_shadowDepthLayerViews;

	Camera* m_camera;
	std::vector<vk::Buffer> m_uniformBuffers;
	std::vector<vma::Allocation> m_uniformBuffersAllocations;

public:
	ShadowCascadeRenderPass(VulkanContext* context, Camera* camera);
	virtual ~ShadowCascadeRenderPass();
	void createFramebuffer()override;
	void createAttachments()override;
	void cleanAttachments()override;
	void createDescriptorPool()override;
	void createDescriptorSetLayout()override;
	void createDescriptorSet(VulkanScene* scene)override;
	void createPipelineLayout()override;
	void createDefaultPipeline()override;
	void createPipelineRessources()override;
	void createPushConstantsRanges()override;
	void updatePipelineRessources(uint32_t currentFrame)override;
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t currentFrame, std::vector<VulkanScene*> scenes)override;
private:
	void createUniformBuffer();
	void updateUniformBuffer(uint32_t currentFrame);

};