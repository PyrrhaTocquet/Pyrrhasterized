#pragma once
#include "ShadowRenderPass.h"
#include "Camera.h"

struct CascadeUniformObject {
	glm::mat4 cascadeViewProjMat[4];
	float cascadeSplits[4];
};

class ShadowCascadeRenderPass : public ShadowRenderPass {
private:
	std::vector<vk::ImageView> m_shadowDepthLayerViews;

	Camera* m_camera;
	std::vector<vk::Buffer> m_uniformBuffers;
	std::vector<vma::Allocation> m_uniformBuffersAllocations;

	std::array<CascadeUniformObject, MAX_FRAMES_IN_FLIGHT> m_cascadeUbos;

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
	void recreateRenderPass() override;
	CascadeUniformObject getCurrentUbo(uint32_t currentFrame);
private:
	void createUniformBuffer();
	void updateUniformBuffer(uint32_t currentFrame);


	void recordShadowCascadeMemoryDependency(vk::CommandBuffer commandBuffer);
};