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


struct ModelLoadingInfo {
	std::filesystem::path path;
	Transform transform;
};

class VulkanScene : Drawable
{
public :
	vk::Buffer m_vertexBuffer, m_indexBuffer = VK_NULL_HANDLE;
	std::vector<Model*> m_models; //TODO private after drawScene refactoring ??
	std::vector<Light*> m_lights;
private:
	VulkanContext* m_context;

	vma::Allocation m_vertexBufferAllocation, m_indexBufferAllocation;
	vma::Allocator m_allocator;
	DirectionalLight* m_sun;
	std::vector<ModelLoadingInfo> m_modelLoadingInfos;
public:
	VulkanScene(VulkanContext* context, DirectionalLight* sun);
	~VulkanScene();
	void addModel(const std::filesystem::path& path, const Transform& transform);
	void addModel(Model* model);
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

};

