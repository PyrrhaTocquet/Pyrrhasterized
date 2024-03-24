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
	vk::Buffer m_meshletInfoBuffer, m_primitiveBuffer, m_indexBuffer, m_vertexBuffer = VK_NULL_HANDLE;
	std::vector<Model*> m_models; //TODO private after drawScene refactoring ??
	std::vector<Light*> m_lights;
private:
	VulkanContext* m_context;

	vma::Allocation m_meshletInfoBufferAllocation, m_primitiveBufferAllocation, m_indexBufferAllocation, m_vertexBufferAllocation;
	uint32_t m_meshletCount = 0, m_primitiveCount = 0, m_indexCount = 0, m_vertexCount = 0;
	vma::Allocator* m_allocator;
	DirectionalLight* m_sun;
	std::vector<ModelLoadingInfo> m_modelLoadingInfos;


	vk::DescriptorPool m_geometryDescriptorPool;
	vk::DescriptorSet m_geometryDescriptorSet;
public:
	VulkanScene(VulkanContext* context, DirectionalLight* sun);
	~VulkanScene();
	void addModel(const std::filesystem::path& path, const Transform& transform);
	void addModel(Model* model);
	void createGeometryDescriptorSet(vk::DescriptorSetLayout geometryDescriptorSetLayout);
	[[nodiscard]] vk::DescriptorSet getGeometryDescriptorSet();
	void loadModels();
	void addEntity(Entity* entity);
	void createGeometryBuffers();
	[[nodiscard]]const uint32_t getIndexBufferSize();
	void addLight(Light* light);
	[[nodiscard]]std::vector<Light*> getLights();
	void updateLights();
	[[nodiscard]] DirectionalLight* getSun();
	void draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant) override;
private:
	void createVertexBuffer();
	void createIndexBuffer();
	void createMeshletInfoBuffer();
	void createPrimitivesBuffer();

};

