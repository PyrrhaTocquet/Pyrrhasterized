#include "VulkanScene.h"

#include <unordered_map>


VulkanScene::VulkanScene(VulkanContext* context, DirectionalLight* sun) {
	m_allocator = context->getAllocator();
	m_context = context;
	m_sun = sun;
	m_sun->setShadowCaster();
	addLight(sun);
}

VulkanScene::~VulkanScene()
{
	m_allocator.destroyBuffer(m_indexBuffer, m_indexBufferAllocation);
	m_allocator.destroyBuffer(m_vertexBuffer, m_vertexBufferAllocation);

	for (const auto& model : m_models) {
		delete model;
	}

}

/* Disabled because no support for a scene graph yet.
//adds the inputed scene to the list of children scenes
void VulkanScene::addChildren(VulkanScene* childrenScene) {
	if (childrenScene == nullptr) {
		throw std::runtime_error("Children scene is nullptr !");
	}
	m_childrenScenes.push_back(childrenScene);
}*/

//Adds the model information to the model information list for further loading
void VulkanScene::addModel(const std::filesystem::path& path, const Transform& transform)
{
	ModelLoadingInfo modelInfo{
		.path = path,
		.transform = transform,
	};
	m_modelLoadingInfos.push_back(modelInfo);
}

//Adds the model to the model list
void VulkanScene::addModel(Model* model)
{
	m_models.push_back(model);
}


//Function used in order to multithread model loading
static std::mutex modelsMutex;
static void newModel(VulkanContext* context, std::filesystem::path path, Transform transform, std::vector<Model*>* models) {
	Model* model = new Model(context, path, transform);
	std::lock_guard<std::mutex> lock(modelsMutex);
	models->push_back(model);
};

//Loads the models in the model info list (multithreaded)
void VulkanScene::loadModels()
{
	std::vector<std::jthread> modelLoadingThreads;
	modelLoadingThreads.resize(m_modelLoadingInfos.size());
	for (uint32_t i = 0; i < m_modelLoadingInfos.size(); i++)
	{
		modelLoadingThreads[i] = std::jthread(newModel, m_context, m_modelLoadingInfos[i].path, m_modelLoadingInfos[i].transform, &m_models);
	}
}

//Adds the entity modl to the scene
void VulkanScene::addEntity(Entity* entity) {
	addModel(entity->getModelPtr());
}

//Creates the index and vertex buffer
void VulkanScene::createGeometryBuffers()
{
	std::jthread vertexBufferThread(&VulkanScene::createVertexBuffer, this);
	std::jthread indexBufferThread(&VulkanScene::createIndexBuffer, this);
}

//Computes the index buffer size from indices count
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

void VulkanScene::addLight(Light* light)
{
	if (m_lights.size() == MAX_LIGHT_COUNT)
	{
		std::cout << "Light not added, maximum light count reached" << std::endl;
		return;
	}
	m_lights.push_back(light);
}

std::vector<Light*> VulkanScene::getLights()
{
	return m_lights;
}

void VulkanScene::updateLights()
{
	for (auto& light : m_lights)
	{
		light->update();
	}
}

DirectionalLight* VulkanScene::getSun()
{
	return m_sun;
}

void VulkanScene::draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant)
{
	VkDeviceSize offset = 0;
	commandBuffer.bindVertexBuffers(0, 1, &m_vertexBuffer, &offset);
	commandBuffer.bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint32);

	uint32_t indexOffset = 0;
	//Draws each model in a scene
	for (auto& model : m_models) {
		model->drawModel(commandBuffer, pipelineLayout, indexOffset, pushConstant);
	}
}

//creates the index buffer
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


//creates the vertex buffer
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

