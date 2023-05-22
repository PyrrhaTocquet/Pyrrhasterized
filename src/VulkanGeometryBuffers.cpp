#include "VulkanGeometryBuffers.h"
//#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

VulkanGeometryBuffers::~VulkanGeometryBuffers()
{
	m_allocator.destroyBuffer(m_indexBuffer, m_indexBufferAllocation);
	m_allocator.destroyBuffer(m_vertexBuffer, m_vertexBufferAllocation);
}

VulkanGeometryBuffers::VulkanGeometryBuffers(VulkanContext* context)
{

	m_allocator = context->getAllocator();

	loadObj("/home/pyrrha/Programming/Vulkan/Vulkan_Base_Project/assets/PikachuObj/model.obj");

	
	createVertexBuffer(context);
	createIndexBuffer(context);

}

uint32_t VulkanGeometryBuffers::getIndexBufferSize()
{
	return m_indices.size();
}


void VulkanGeometryBuffers::loadObj(const std::string path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
		throw std::runtime_error(warn + err);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};
			//array of float values instead of glm::vec3 so you have to multiply by 3
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				-attrib.vertices[3 * index.vertex_index + 2], //Z up
				attrib.vertices[3 * index.vertex_index + 1],
			};
			
			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				-attrib.normals[3 * index.normal_index + 2],
				attrib.normals[3 * index.normal_index + 1]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			m_vertices.push_back(vertex);
			m_indices.push_back(m_indices.size());
		}
	}
}

void VulkanGeometryBuffers::createVertexBuffer(VulkanContext* context) {

	vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	vk::Buffer stagingBuffer;
	vma::Allocation stagingAllocation;
	std::tie(stagingBuffer, stagingAllocation) = context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vma::MemoryUsage::eCpuToGpu);

	void* data = m_allocator.mapMemory(stagingAllocation);
	memcpy(data, m_vertices.data(), (size_t)bufferSize);
	m_allocator.unmapMemory(stagingAllocation);

	std::tie(m_vertexBuffer, m_vertexBufferAllocation) = context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vma::MemoryUsage::eGpuOnly);

	context->copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	m_allocator.destroyBuffer(stagingBuffer, stagingAllocation);

}

void VulkanGeometryBuffers::createIndexBuffer(VulkanContext* context) {

	vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

	vk::Buffer stagingBuffer;
	vma::Allocation stagingAllocation;
	std::tie(stagingBuffer, stagingAllocation) = context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vma::MemoryUsage::eCpuToGpu);

	void* data = m_allocator.mapMemory(stagingAllocation);
	memcpy(data, m_indices.data(), (size_t)bufferSize);
	m_allocator.unmapMemory(stagingAllocation);

	std::tie(m_indexBuffer, m_indexBufferAllocation) = context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vma::MemoryUsage::eGpuOnly);

	context->copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	m_allocator.destroyBuffer(stagingBuffer, stagingAllocation);

}