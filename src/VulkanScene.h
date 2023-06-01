#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "Defs.h"
#include "Drawable.h"
#include <filesystem>
#include "Model.h"


class VulkanScene : Drawable
{
public :
	vk::Buffer m_vertexBuffer, m_indexBuffer = VK_NULL_HANDLE;

	std::vector<vk::DescriptorSet> m_descriptorSets;
	std::vector<Model*> m_models; //TODO private after drawScene refactoring ??
private:
	std::vector<VulkanScene*> m_childrenScenes;

	VulkanContext* m_context;

	vma::Allocation m_vertexBufferAllocation, m_indexBufferAllocation;
	vma::Allocator m_allocator;


public:
	VulkanScene(VulkanContext* context);
	~VulkanScene();
	void addChildren(VulkanScene* childrenScene);
	void addModel(const std::filesystem::path& path, const Transform& transform);
	void createBuffers();
	void setDescriptorSets(std::vector<vk::DescriptorSet> descriptorSet);
	const uint32_t getIndexBufferSize();

	void draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout ) override;
private:
	void createVertexBuffer();
	void createIndexBuffer();

};

