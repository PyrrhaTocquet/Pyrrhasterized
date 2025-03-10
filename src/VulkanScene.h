#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "Defs.h"
#include "Entity.h"
#include <filesystem>
#include "Model.h"
#include "Drawable.h"
#include "DirectionalLight.h"
#include <future>
#include <thread>


class VulkanScene : Drawable
{
public :
	VulkanBuffer m_meshletInfoBuffer, m_primitiveBuffer, m_indexBuffer, m_vertexBuffer;
	std::vector<Model*> m_models; //TODO private after drawScene refactoring ??
	std::vector<Light*> m_lights;

	// Uniform Buffers
	std::vector<VulkanBuffer> m_generalUniformBuffers;
	std::vector<VulkanBuffer> m_lightUniformBuffers;
	std::vector<VulkanBuffer> m_shadowCascadeUniformBuffers;
	std::array<std::vector<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_materialUniformBuffers;

	uint32_t m_materialCount = 0;
private:
	VulkanContext* m_context;

	uint32_t m_meshletCount = 0, m_primitiveCount = 0, m_indexCount = 0, m_vertexCount = 0;
	vma::Allocator* m_allocator;
	DirectionalLight* m_sun;
	std::vector<ModelLoadingInfo> m_modelLoadingInfos;
	std::array<CascadeUniformObject, MAX_FRAMES_IN_FLIGHT> m_cascadeUbos;

	Camera* m_camera;

	vk::DescriptorPool m_geometryDescriptorPool;
	vk::DescriptorSet m_geometryDescriptorSet;
public:
	VulkanScene(VulkanContext* context, DirectionalLight* sun);
	~VulkanScene();
	void addModel(const std::filesystem::path& path, const Transform& transform);
	void addModel(Model* model);
	void createGeometryDescriptorSet(vk::DescriptorSetLayout geometryDescriptorSetLayout);
	[[nodiscard]]	vk::DescriptorSet getGeometryDescriptorSet();
	void loadModels();
	void addEntity(Entity* entity);
	void createGeometryBuffers();
	[[nodiscard]]	const uint32_t getIndexBufferSize();
	void addLight(Light* light);
	[[nodiscard]]	std::vector<Light*> getLights();
	void updateLights();
	[[nodiscard]]	DirectionalLight* getSun();
	void draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant) override;
	void	createUniformBuffers();
	void	updateUniformBuffers(uint32_t m_currentFrame);
	void	setCamera(Camera *camera);

	[[nodiscard]]	const VulkanBuffer getGeneralUniformBuffer(uint32_t currentFrame) { return m_generalUniformBuffers[currentFrame]; };
	[[nodiscard]] const VulkanBuffer getLightUniformBuffer(uint32_t currentFrame) {
		return m_lightUniformBuffers[currentFrame];
	};
	[[nodiscard]] const VulkanBuffer getShadowCascadeUniformBuffer(uint32_t currentFrame) {
		return m_shadowCascadeUniformBuffers[currentFrame];
	};
	[[nodiscard]] const VulkanBuffer getMaterialUniformBuffer(uint32_t currentFrame, uint32_t materialId) {
		return m_materialUniformBuffers[currentFrame][materialId]; // Not sure it's the material id here
	};
	[[nodiscard]]	std::vector<vk::DescriptorImageInfo> generateTextureImageInfo();
private:
	void createVertexBuffer();
	void createIndexBuffer();
	void updateGeneralUniformBuffer(uint32_t currentFrame);
	void updateLightUniformBuffer(uint32_t currentFrame);
	void updateShadowCascadeUniformBuffer(uint32_t currentFrame);
};

