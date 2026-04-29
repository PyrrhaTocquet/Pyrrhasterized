/*
author: Pyrrha Tocquet
date: 10/05/26
desc: Compute pass that handles frustum tiling and light list generation for forward+ light culling
*/

#pragma once
#include "VulkanComputePass.h"

class LightCullingPass : public VulkanComputePass
{
protected:
	struct FrustumGridDispatchParams
	{
		uint32_t threadGroupsCount[2]{};
		uint32_t executedThreadsCount[2]{}; // <= executed threads, depends on screen res 
	};

	struct Plane
    {
		float N[3]{}; // normal
		float d{};  // distance to origin
    };

    struct Frustum
    {
		Plane planes[4]{}; // lleft, right, top, bottom
    };

	vk::Extent2D	m_extent{};
	std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT>	m_dispatchParamsBuffer{};
	std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT>	m_outFrustums{};
	uint32_t m_dispatchGroupCount[2]{1u, 1u};
	uint32_t m_threadsDispatched[2]{1u, 1u};
public:
	LightCullingPass() = default;
	LightCullingPass(VulkanContext* context);
	LightCullingPass(const LightCullingPass&) = delete;
	LightCullingPass operator=(const LightCullingPass&) = delete;
	LightCullingPass(LightCullingPass&&) = delete;
	LightCullingPass operator=(LightCullingPass&&) = delete;
	virtual ~LightCullingPass();

	virtual void recreatePass() override;
	virtual void executePass(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex, uint32_t m_currentFrame, std::vector<VulkanScene*> scenes) override;
	void updateBuffers();

	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createPipelineLayout();
	void createDefaultPipeline();
	void createDescriptorSets(VulkanScene* scene, std::vector<vk::DescriptorImageInfo> textureImageInfos) override;
	void updatePipelineRessources(uint32_t currentFrame, std::vector<VulkanScene*>)override;

private:
	void createBuffers();
	void updateDispatchParams();
};