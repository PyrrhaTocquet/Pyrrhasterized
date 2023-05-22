#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanContext.h"
#include "glm/glm.hpp"




class VulkanGeometryBuffers
{

struct Vertex {
	glm::vec3 pos;
	glm::vec2 texCoord;
	glm::vec3 normal;
};

public: 
	vk::Buffer m_vertexBuffer, m_indexBuffer;

	VulkanGeometryBuffers();
	~VulkanGeometryBuffers();
	VulkanGeometryBuffers(VulkanContext* context);
	uint32_t getIndexBufferSize();


private:
	vma::Allocator m_allocator;
	vma::Allocation m_vertexBufferAllocation, m_indexBufferAllocation;
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	
	void loadObj(const std::string path);
	void createVertexBuffer(VulkanContext* context);
	void createIndexBuffer(VulkanContext* context);
};

