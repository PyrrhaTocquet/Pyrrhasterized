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

	//Camera* m_camera;


public:
	ShadowCascadeRenderPass(VulkanContext* context, Camera* camera);
	virtual ~ShadowCascadeRenderPass();
	void createFramebuffer()override;
	void createAttachments()override;
	void cleanAttachments()override;
	void drawRenderPass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t currentFrame, vk::DescriptorSet descriptorSet, vk::Pipeline pipeline, std::vector<VulkanScene*> scenes, vk::PipelineLayout pipelineLayout)override;


};