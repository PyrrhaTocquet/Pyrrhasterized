#include "VulkanScene.h"

#include <unordered_map>

VulkanScene::VulkanScene(VulkanContext* context) {
	m_allocator = context->getAllocator();
	m_context = context;
}

VulkanScene::~VulkanScene()
{
	m_allocator.destroyBuffer(m_indexBuffer, m_indexBufferAllocation);
	m_allocator.destroyBuffer(m_vertexBuffer, m_vertexBufferAllocation);

	for (const auto& model : m_models) {
		delete model;
	}

}

void VulkanScene::addChildren(VulkanScene* childrenScene) {
	if (childrenScene == nullptr) {
		throw std::runtime_error("Children scene is nullptr !");
	}
	m_childrenScenes.push_back(childrenScene);
}

void VulkanScene::addModel(const std::filesystem::path& path, const Transform& transform)
{
	Model* model = new Model(m_context, path, transform);
	m_models.push_back(model);
}

void VulkanScene::addModel(Model* model)
{
	m_models.push_back(model);
}


void VulkanScene::addEntity(Entity* entity) {
	addModel(entity->getModelPtr());
}

void VulkanScene::createBuffers()
{
	createVertexBuffer();
	createIndexBuffer();
}


const uint32_t VulkanScene::getIndexBufferSize()
{
	uint32_t indicesCount = 0;
	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model->getMeshes()) {
			indicesCount += texturedMesh.loadingIndices.size();
		}

	}
	return indicesCount;
}

void VulkanScene::draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant)
{
	static auto startTime = std::chrono::high_resolution_clock::now(); //Todo, engine solution for time

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - startTime).count();
	pushConstant.time = time;

	VkDeviceSize offset = 0;
	commandBuffer.bindVertexBuffers(0, 1, &m_vertexBuffer, &offset);
	commandBuffer.bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint32);

	uint32_t indexOffset = 0;
	//Draws each model in a scene
	for (auto& model : m_models) {
		model->drawModel(commandBuffer, pipelineLayout, time, indexOffset, pushConstant);
	}
}

void VulkanScene::createIndexBuffer() 
{
	uint32_t indicesCount = getIndexBufferSize();

	vk::DeviceSize bufferSize = sizeof(Vertex) * indicesCount;

	vk::Buffer stagingBuffer;
	vma::Allocation stagingAllocation;
	std::tie(stagingBuffer, stagingAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vma::MemoryUsage::eCpuToGpu);

	char* data = static_cast<char*>(m_allocator.mapMemory(stagingAllocation));

	int indexOffset = 0;
	for (int modelIndex = 0; modelIndex < m_models.size(); modelIndex++) {
		auto texturedMeshes = m_models[modelIndex]->getMeshes();
		for (int texturedMeshIndex = 0; texturedMeshIndex < texturedMeshes.size(); texturedMeshIndex++) {
			
			auto& texturedMesh = texturedMeshes[texturedMeshIndex];
			for (auto& index : texturedMesh.loadingIndices) {
				index += indexOffset;
			}
			indexOffset += texturedMesh.loadingIndices.size();
			memcpy(data, texturedMesh.loadingIndices.data(), static_cast<size_t>(texturedMesh.loadingIndices.size()*sizeof(uint32_t)));
			data += texturedMesh.loadingIndices.size() * sizeof(uint32_t);
		}
		m_models[modelIndex]->clearLoadingIndexData();
	}
	m_allocator.unmapMemory(stagingAllocation);

	std::tie(m_indexBuffer, m_indexBufferAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vma::MemoryUsage::eGpuOnly);

	m_context->copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	m_allocator.destroyBuffer(stagingBuffer, stagingAllocation);
}



void VulkanScene::createVertexBuffer() 
{
	size_t verticesCount = 0;
	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model->getMeshes()) {
			verticesCount += texturedMesh.loadingVertices.size();
		}
		
	}

	vk::DeviceSize bufferSize = sizeof(Vertex) * verticesCount;

	vk::Buffer stagingBuffer;
	vma::Allocation stagingAllocation;
	std::tie(stagingBuffer, stagingAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vma::MemoryUsage::eCpuToGpu);

	char* data = static_cast<char*>(m_allocator.mapMemory(stagingAllocation));

	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model->getMeshes()) {
			memcpy(data, texturedMesh.loadingVertices.data(), static_cast<size_t>(texturedMesh.loadingVertices.size() * sizeof(Vertex)));
			data += texturedMesh.loadingVertices.size() * sizeof(Vertex);
		}
		model->clearLoadingVertexData();
	}
	m_allocator.unmapMemory(stagingAllocation);

	std::tie(m_vertexBuffer, m_vertexBufferAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vma::MemoryUsage::eGpuOnly);

	m_context->copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	m_allocator.destroyBuffer(stagingBuffer, stagingAllocation);
}

