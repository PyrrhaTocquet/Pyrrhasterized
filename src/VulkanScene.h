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
private:
	std::vector<VulkanScene*> m_childrenScenes;

	VulkanContext* m_context;

	vma::Allocation m_vertexBufferAllocation, m_indexBufferAllocation;
	vma::Allocator m_allocator;

	std::vector<ModelLoadingInfo> m_modelLoadingInfos;
public:
	VulkanScene(VulkanContext* context);
	~VulkanScene();
	void addChildren(VulkanScene* childrenScene);
	void addModel(const std::filesystem::path& path, const Transform& transform);
	void addModel(Model* model);
	void loadModels();
	void addEntity(Entity* entity);
	void createBuffers();
	[[nodiscard]]const uint32_t getIndexBufferSize();

	void draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant) override;
private:
	void createVertexBuffer();
	void createIndexBuffer();

};

