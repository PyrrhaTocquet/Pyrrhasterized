#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include "VulkanContext.h"
#include "VulkanImage.h"


//TODO Better Vertex Data managment
struct Vertex {
	glm::vec3 pos;
	glm::vec2 texCoord;
	glm::vec3 normal;


	static vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription bindingDescription;
		bindingDescription.binding = 0,
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;
		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);



		return attributeDescriptions;
	}
};

struct TexturedMesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VulkanImage* textureImage;
	uint32_t textureId;
};

struct Model {
	std::vector<TexturedMesh> texturedMeshes;
	glm::mat4 matrix;
};

class VulkanScene
{
public :
	vk::Buffer m_vertexBuffer, m_indexBuffer = VK_NULL_HANDLE;

	std::vector<vk::DescriptorSet> m_descriptorSets;
	std::vector<Model> m_models; //TODO private after drawScene refactoring ??
private:
	std::vector<VulkanScene*> m_childrenScenes;

	VulkanContext* m_context;


	vma::Allocation m_vertexBufferAllocation, m_indexBufferAllocation;
	vma::Allocator m_allocator;


public:
	VulkanScene(VulkanContext* context);
	~VulkanScene();
	void addChildren(VulkanScene* childrenScene);
	void addObjModel(const std::string& path, const glm::mat4& modelMatrix);
	void createBuffers();
	void setDescriptorSets(std::vector<vk::DescriptorSet> descriptorSet);
	const uint32_t getIndexBufferSize();
private:
	void createVertexBuffer();
	void createIndexBuffer();
};
