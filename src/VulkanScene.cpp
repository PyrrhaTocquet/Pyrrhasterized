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
	m_allocator->destroyBuffer(m_indexBuffer, m_indexBufferAllocation);
	m_allocator->destroyBuffer(m_vertexBuffer, m_vertexBufferAllocation);
	m_allocator->destroyBuffer(m_primitiveBuffer, m_primitiveBufferAllocation);
	m_allocator->destroyBuffer(m_meshletInfoBuffer, m_meshletInfoBufferAllocation);

	for (const auto& model : m_models) {
		delete model;
	}
	//No need to delete lights, they are deleted as entities

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



void VulkanScene::createGeometryDescriptorSet(vk::DescriptorSetLayout geometryDescriptorSetLayout)
{
	 vk::Device device = m_context->getDevice();
	//Descriptor Pool
    {
		vk::DescriptorPoolSize bindingPoolSize {.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1};
        std::array<vk::DescriptorPoolSize, 4> poolSizes{bindingPoolSize, bindingPoolSize, bindingPoolSize, bindingPoolSize};
      
        vk::DescriptorPoolCreateInfo poolInfo{
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
        };
        try {
            m_geometryDescriptorPool = device.createDescriptorPool(poolInfo);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("could not create descriptor pool");
        }
    }

    /*Descriptor Sets Allocation*/
    vk::DescriptorSetAllocateInfo allocInfo = {
     .descriptorPool = m_geometryDescriptorPool,
     .descriptorSetCount = 1,
     .pSetLayouts = &geometryDescriptorSetLayout,
    };

    try {
        m_geometryDescriptorSet = m_context->getDevice().allocateDescriptorSets(allocInfo)[0];
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("could not allocate descriptor sets");
    }

	//Writing to the set
    vk::DescriptorBufferInfo meshletBufferInfo{
        .buffer = m_meshletInfoBuffer,
        .offset = 0,
        .range = sizeof(MeshletIndexingInfo) * m_meshletCount
    };

    vk::DescriptorBufferInfo primitiveBufferInfo{
        .buffer = m_primitiveBuffer,
        .offset = 0,
        .range = sizeof(Meshlet::Triangle) * m_primitiveCount,
    };

	vk::DescriptorBufferInfo indexBufferInfo{
		.buffer = m_indexBuffer,
		.offset = 0,
		.range = sizeof(uint32_t) * m_indexCount,
	};

	vk::DescriptorBufferInfo vertexBufferInfo{
		.buffer = m_vertexBuffer,
		.offset = 0,
		.range = sizeof(Vertex) * m_vertexCount,
	};

	vk::WriteDescriptorSet meshletBufferDescriptorWrite{
		.dstSet = m_geometryDescriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.pBufferInfo = &meshletBufferInfo,
	};

	vk::WriteDescriptorSet primitiveBufferWrite = meshletBufferDescriptorWrite;
	primitiveBufferWrite.dstBinding = 1;
	primitiveBufferWrite.pBufferInfo = &primitiveBufferInfo;

	vk::WriteDescriptorSet indexBufferWrite = meshletBufferDescriptorWrite;
	indexBufferWrite.dstBinding = 2;
	indexBufferWrite.pBufferInfo = &indexBufferInfo;

	vk::WriteDescriptorSet vertexBufferWrite = meshletBufferDescriptorWrite;
	vertexBufferWrite.dstBinding = 3;
	vertexBufferWrite.pBufferInfo = &vertexBufferInfo;

	std::array<vk::WriteDescriptorSet, 4> descriptorWrites{meshletBufferDescriptorWrite, primitiveBufferWrite, indexBufferWrite, vertexBufferWrite};

    try {
        m_context->getDevice().updateDescriptorSets(descriptorWrites, nullptr);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor sets");
    }
    
}

vk::DescriptorSet VulkanScene::getGeometryDescriptorSet()
{
    return m_geometryDescriptorSet;
}

//Function used in order to multithread model loading
static std::mutex modelsMutex;
static void newModel(VulkanContext* context, std::filesystem::path path, Transform transform, std::vector<Model*>* models) {
	Model* model = new Model(context, path, transform);
	std::lock_guard<std::mutex> lock(modelsMutex);
	models->push_back(model);
};

// Loads the models in the model info list (multithreaded)
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

template <typename T>
void copyStdVectorToGPUBuffer(VulkanContext* context, vma::Allocator* allocator, const std::vector<T>& inputStdVector, vk::Buffer gpuBuffer, vk::DeviceSize size)
{
	assert(inputStdVector.size() * sizeof(T) == size);

	vk::Buffer stagingBuffer;
	vma::Allocation stagingAllocation;

	std::tie(stagingBuffer, stagingAllocation) = context->createBuffer(size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuToGpu, "std::vector to GPU buffer staging buffer");

	char* data = static_cast<char*>(allocator->mapMemory(stagingAllocation));
	memcpy(data, inputStdVector.data(), size);
	allocator->unmapMemory(stagingAllocation);
	context->copyBuffer(stagingBuffer, gpuBuffer, size);
	allocator->destroyBuffer(stagingBuffer, stagingAllocation);
}

//Creates the index and vertex buffer
void VulkanScene::createGeometryBuffers()
{
	/* Buffers Sizes*/
	for(auto model: m_models)
	{
		for(auto mesh: model->getMeshes())
		{
			m_meshletCount += mesh.meshlets.size();
			m_vertexCount += mesh.vertices.size();

			for(auto meshlet: mesh.meshlets)
			{
				m_primitiveCount += meshlet.primitiveIndices.size();
				m_indexCount += meshlet.uniqueVertexIndices.size();
			}
		}
	}

	vk::DeviceSize meshletBufferSize = sizeof(MeshletIndexingInfo) * m_meshletCount;
	vk::DeviceSize primitiveBufferSize = sizeof(Meshlet::Triangle) * m_primitiveCount;
	vk::DeviceSize indexBufferSize = sizeof(uint32_t) * m_indexCount;
	vk::DeviceSize vertexBufferSize = sizeof(Vertex) * m_vertexCount;

	/* Buffers Creation */
	std::tie(m_meshletInfoBuffer, m_meshletInfoBufferAllocation) = m_context->createBuffer(meshletBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eGpuOnly, "Meshlet Info Buffer");
	std::tie(m_primitiveBuffer, m_primitiveBufferAllocation) = m_context->createBuffer(primitiveBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eGpuOnly, "Primitive Buffer");
	std::tie(m_indexBuffer, m_indexBufferAllocation) = m_context->createBuffer(indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eGpuOnly, "Index Buffer");
	std::tie(m_vertexBuffer, m_vertexBufferAllocation) = m_context->createBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eGpuOnly, "Vertex Buffer");

	/* Filling the buffer */
	MeshletIndexingInfo meshletInfo{};

	std::vector<MeshletIndexingInfo> meshletInfos;
	std::vector<Meshlet::Triangle> triangles;
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	uint32_t i = 0;
	for(auto& model: m_models)
	{
		for (auto& mesh: model->getMeshes())
		{			

			for(auto& meshlet: mesh.meshlets)
			{
				meshletInfo.boundingSphere = meshlet.meshletInfo.boundingSphere;
				meshletInfo.primitiveOffset = triangles.size();	
				
				triangles.insert(triangles.end(), meshlet.primitiveIndices.begin(), meshlet.primitiveIndices.end());
				
				std::for_each(meshlet.uniqueVertexIndices.begin(), meshlet.uniqueVertexIndices.end(), [indices](uint32_t& index){index+=indices.size();});
				indices.insert(indices.end(), meshlet.uniqueVertexIndices.begin(), meshlet.uniqueVertexIndices.end());
				


				meshletInfo.primitiveCount = meshlet.primitiveIndices.size();
				meshletInfo.vertexCount = meshlet.uniqueVertexIndices.size();
				
				
				meshletInfo.meshletId = i;

				meshlet.meshletInfo = meshletInfo;
				
				meshletInfos.push_back(meshletInfo);
				meshletInfo.vertexOffset += meshlet.uniqueVertexIndices.size();
				

				i++;
			}
			
			vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
			
		}
	} 

	/* Sending to GPU buffers */
	copyStdVectorToGPUBuffer<MeshletIndexingInfo>(m_context, m_allocator, meshletInfos, m_meshletInfoBuffer, meshletBufferSize);
	copyStdVectorToGPUBuffer<Meshlet::Triangle>(m_context, m_allocator, triangles, m_primitiveBuffer,  primitiveBufferSize);
	copyStdVectorToGPUBuffer<uint32_t>(m_context, m_allocator, indices, m_indexBuffer,  indexBufferSize);
	copyStdVectorToGPUBuffer<Vertex>(m_context, m_allocator, vertices, m_vertexBuffer,  vertexBufferSize);

}

//Computes the index buffer size from indices count
const uint32_t VulkanScene::getIndexBufferSize()
{
	uint32_t indicesCount = 0;
	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model->getRawMeshes()) {
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
		vma::MemoryUsage::eCpuToGpu, "Index Staging Buffer");

	char* data = static_cast<char*>(m_allocator->mapMemory(stagingAllocation));

	int indexOffset = 0;
	for (int modelIndex = 0; modelIndex < m_models.size(); modelIndex++) {
		auto texturedMeshes = m_models[modelIndex]->getRawMeshes();
		for (int texturedMeshIndex = 0; texturedMeshIndex < texturedMeshes.size(); texturedMeshIndex++) {
			
			auto& texturedMesh = texturedMeshes[texturedMeshIndex];
			for (auto& index : texturedMesh.loadingIndices) {
				index += indexOffset;
			}
			indexOffset += texturedMesh.loadingIndices.size();
			memcpy(data, texturedMesh.loadingIndices.data(), static_cast<size_t>(texturedMesh.loadingIndices.size()*sizeof(uint32_t)));
			data += texturedMesh.loadingIndices.size() * sizeof(uint32_t);
		}
		//m_models[modelIndex]->clearLoadingIndexData();
	}
	m_allocator->unmapMemory(stagingAllocation);

	std::tie(m_indexBuffer, m_indexBufferAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vma::MemoryUsage::eGpuOnly, "Index Buffer");

	m_context->copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	m_allocator->destroyBuffer(stagingBuffer, stagingAllocation);
}


//creates the vertex buffer
void VulkanScene::createVertexBuffer() 
{
	size_t verticesCount = 0;
	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model->getRawMeshes()) {
			verticesCount += texturedMesh.loadingVertices.size();
		}
		
	}

	vk::DeviceSize bufferSize = sizeof(Vertex) * verticesCount;

	vk::Buffer stagingBuffer;
	vma::Allocation stagingAllocation;
	std::tie(stagingBuffer, stagingAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vma::MemoryUsage::eCpuToGpu, "Vertex Staging Buffer");

	char* data = static_cast<char*>(m_allocator->mapMemory(stagingAllocation));

	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model->getRawMeshes()) {
			memcpy(data, texturedMesh.loadingVertices.data(), static_cast<size_t>(texturedMesh.loadingVertices.size() * sizeof(Vertex)));
			data += texturedMesh.loadingVertices.size() * sizeof(Vertex);
		}
		//model->clearLoadingVertexData();
	}
	m_allocator->unmapMemory(stagingAllocation);

	std::tie(m_vertexBuffer, m_vertexBufferAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vma::MemoryUsage::eGpuOnly, "Vertex Buffer");

	m_context->copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	m_allocator->destroyBuffer(stagingBuffer, stagingAllocation);
}

