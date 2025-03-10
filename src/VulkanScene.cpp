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
	// TODO less verbose stuff
	m_allocator->destroyBuffer(m_indexBuffer.m_Buffer, m_indexBuffer.m_Allocation);
	m_allocator->destroyBuffer(m_vertexBuffer.m_Buffer, m_vertexBuffer.m_Allocation);
	m_allocator->destroyBuffer(m_primitiveBuffer.m_Buffer, m_primitiveBuffer.m_Allocation);
	m_allocator->destroyBuffer(m_meshletInfoBuffer.m_Buffer, m_meshletInfoBuffer.m_Allocation);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_context->getAllocator()->destroyBuffer(m_generalUniformBuffers[i].m_Buffer, m_generalUniformBuffers[i].m_Allocation);
		m_context->getAllocator()->destroyBuffer(m_lightUniformBuffers[i].m_Buffer, m_lightUniformBuffers[i].m_Allocation);
		m_context->getAllocator()->destroyBuffer(m_shadowCascadeUniformBuffers[i].m_Buffer, m_shadowCascadeUniformBuffers[i].m_Allocation);

		for (uint32_t k = 0; k < m_materialUniformBuffers[i].size();k++)
		{
			m_context->getAllocator()->destroyBuffer(m_materialUniformBuffers[i][k].m_Buffer, m_materialUniformBuffers[i][k].m_Allocation);
		}
	}

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
        .buffer = m_meshletInfoBuffer.m_Buffer,
        .offset = 0,
        .range = sizeof(MeshletIndexingInfo) * m_meshletCount
    };

    vk::DescriptorBufferInfo primitiveBufferInfo{
        .buffer = m_primitiveBuffer.m_Buffer,
        .offset = 0,
        .range = sizeof(Meshlet::Triangle) * m_primitiveCount,
    };

	vk::DescriptorBufferInfo indexBufferInfo{
		.buffer = m_indexBuffer.m_Buffer,
		.offset = 0,
		.range = sizeof(uint32_t) * m_indexCount,
	};

	vk::DescriptorBufferInfo vertexBufferInfo{
		.buffer = m_vertexBuffer.m_Buffer,
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
static void newModel(VulkanContext* context, std::filesystem::path path, Transform transform, std::vector<Model*>* models, int modelId) {
	Model* model = new Model(context, path, transform);
	std::lock_guard<std::mutex> lock(modelsMutex);
	models->at(modelId) = model;
};

// Loads the models in the model info list (multithreaded)
void VulkanScene::loadModels()
{
	size_t modelsCount = m_modelLoadingInfos.size();
	std::vector<std::jthread> modelLoadingThreads;
	modelLoadingThreads.resize(modelsCount);
	m_models.resize(modelsCount);
	for (uint32_t i = 0; i < modelsCount; i++)
	{
		modelLoadingThreads[i] = std::jthread(newModel, m_context, m_modelLoadingInfos[i].path, m_modelLoadingInfos[i].transform, &m_models, i);
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

	VulkanBuffer stagingBuffer;

	stagingBuffer = context->createBuffer(size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuToGpu, "std::vector to GPU buffer staging buffer");

	char* data = static_cast<char*>(allocator->mapMemory(stagingBuffer.m_Allocation));
	memcpy(data, inputStdVector.data(), size);
	allocator->unmapMemory(stagingBuffer.m_Allocation);
	context->copyBuffer(stagingBuffer.m_Buffer, gpuBuffer, size);
	allocator->destroyBuffer(stagingBuffer.m_Buffer, stagingBuffer.m_Allocation);
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
	m_meshletInfoBuffer = m_context->createBuffer(meshletBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eGpuOnly, "Meshlet Info Buffer");
	m_primitiveBuffer= m_context->createBuffer(primitiveBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eGpuOnly, "Primitive Buffer");
	m_indexBuffer = m_context->createBuffer(indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eGpuOnly, "Index Buffer");
	m_vertexBuffer = m_context->createBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eGpuOnly, "Vertex Buffer");

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
	copyStdVectorToGPUBuffer<MeshletIndexingInfo>(m_context, m_allocator, meshletInfos, m_meshletInfoBuffer.m_Buffer, meshletBufferSize);
	copyStdVectorToGPUBuffer<Meshlet::Triangle>(m_context, m_allocator, triangles, m_primitiveBuffer.m_Buffer,  primitiveBufferSize);
	copyStdVectorToGPUBuffer<uint32_t>(m_context, m_allocator, indices, m_indexBuffer.m_Buffer,  indexBufferSize);
	copyStdVectorToGPUBuffer<Vertex>(m_context, m_allocator, vertices, m_vertexBuffer.m_Buffer,  vertexBufferSize);
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

	VulkanBuffer stagingBuffer;
	stagingBuffer = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vma::MemoryUsage::eCpuToGpu, "Index Staging Buffer");

	char* data = static_cast<char*>(m_allocator->mapMemory(stagingBuffer.m_Allocation));

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
	m_allocator->unmapMemory(stagingBuffer.m_Allocation);

	m_indexBuffer = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vma::MemoryUsage::eGpuOnly, "Index Buffer");

	m_context->copyBuffer(stagingBuffer.m_Buffer, m_indexBuffer.m_Buffer, bufferSize);

	m_allocator->destroyBuffer(stagingBuffer.m_Buffer, stagingBuffer.m_Allocation);
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

	VulkanBuffer stagingBuffer;
	stagingBuffer = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vma::MemoryUsage::eCpuToGpu, "Vertex Staging Buffer");

	char* data = static_cast<char*>(m_allocator->mapMemory(stagingBuffer.m_Allocation));

	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model->getRawMeshes()) {
			memcpy(data, texturedMesh.loadingVertices.data(), static_cast<size_t>(texturedMesh.loadingVertices.size() * sizeof(Vertex)));
			data += texturedMesh.loadingVertices.size() * sizeof(Vertex);
		}
		//model->clearLoadingVertexData();
	}
	m_allocator->unmapMemory(stagingBuffer.m_Allocation);

	m_vertexBuffer = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vma::MemoryUsage::eGpuOnly, "Vertex Buffer");

	m_context->copyBuffer(stagingBuffer.m_Buffer, m_vertexBuffer.m_Buffer, bufferSize);

	m_allocator->destroyBuffer(stagingBuffer.m_Buffer, stagingBuffer.m_Allocation);
}

void VulkanScene::createUniformBuffers()
{
	//General UBO
	{
		vk::DeviceSize bufferSize = sizeof(GeneralUniformBufferObject);

		m_generalUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_generalUniformBuffers[i] = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu, "General Uniform Buffer");
		}
	}
	// Lights UBO
	{
		vk::DeviceSize bufferSize = sizeof(LightUBO) * MAX_LIGHT_COUNT;

		m_lightUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_lightUniformBuffers[i] = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu, "Light Uniform Buffer");
		}
	}
	//Shadow Cascades UBO
	{
		vk::DeviceSize bufferSize = sizeof(CascadeUniformObject);

		m_shadowCascadeUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_shadowCascadeUniformBuffers[i] = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu, "Shadow Cascades Uniform Buffer");
		}
	}
	// Materials UBO ( Maybe it needs to be somewhere else )
	{
 		std::vector<MaterialUBO> materialUBOs;
		MaterialUBO defaultMaterial{};
		materialUBOs.push_back(defaultMaterial);
		//Retrieving Material UBOs
		for (auto& model : m_models) {
			for (auto& mesh : model->getRawMeshes()) {
				if (mesh.material != nullptr)
				{
					mesh.materialId = materialUBOs.size();
					materialUBOs.push_back(mesh.material->getUBO());
				}
				else {
					mesh.materialId = 0;
				}

			}
		}

		m_materialCount = materialUBOs.size();

		for (uint32_t currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++) 
		{
			std::vector<vk::DescriptorBufferInfo> materialBufferInfos;

			//Creating and filling the Uniform Buffers for each material
			// TODO Just one buffer for fuck sake
				vk::DeviceSize bufferSize = sizeof(MaterialUBO);
			m_materialUniformBuffers[currentFrame].resize(materialUBOs.size());
			materialBufferInfos.resize(materialUBOs.size());
			for (size_t i = 0; i < materialUBOs.size(); i++)
			{
				m_materialUniformBuffers[currentFrame][i] = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu, "Material Uniform Buffer");

				//Write the material to the buffer
				void* data = m_context->getAllocator()->mapMemory(m_materialUniformBuffers[currentFrame][i].m_Allocation);
				memcpy(data, &materialUBOs[i], sizeof(MaterialUBO));
				m_context->getAllocator()->unmapMemory(m_materialUniformBuffers[currentFrame][i].m_Allocation);
			}
		}
	}
}

//adds the texture info to the texture image info and sets the right Id to the mesh
static void appendImageInfo(std::vector<vk::DescriptorImageInfo>& textureImageInfo, vk::Sampler sampler, VulkanImage* image, uint32_t& id, uint32_t& textureId)
{
	vk::DescriptorImageInfo imageInfo{
			  .sampler = sampler,
			  .imageView = image->m_imageView,
			  .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
	};
	textureImageInfo.push_back(imageInfo);
	id = textureId; //Attributes the texture id to the mesh
	textureId++;
}

//returns are vector of DescriptorImageInfo, containing the albedo and normal map texture information from the Models' TexturedMeshes
[[nodiscard]] std::vector<vk::DescriptorImageInfo> VulkanScene::generateTextureImageInfo()
{
	std::vector<vk::DescriptorImageInfo> textureImageInfo;
	uint32_t textureId = 0;
	for (auto& model : m_models) {
		for (auto& texturedMesh : model->getRawMeshes()) {
			Material* material = texturedMesh.material;
			if (material == nullptr)
				continue;

			if (material->hasAlbedoTexture())
			{
				appendImageInfo(textureImageInfo, Material::s_baseColorSampler, material->getAlbedoTexture(), material->m_albedoTextureId, textureId);
			}
			if (material->hasNormalTexture())
			{
				appendImageInfo(textureImageInfo, Material::s_normalSampler, material->getNormalTexture(), material->m_normalTextureId, textureId);
			}
			if (material->hasMetallicRoughness())
			{
				appendImageInfo(textureImageInfo, Material::s_metallicRoughnessSampler, material->getMetallicRoughnessTexture(), material->m_metallicRoughnessTextureId, textureId);
			}
			if (material->hasEmissiveTexture())
			{
				appendImageInfo(textureImageInfo, Material::s_emissiveSampler, material->getEmissiveTexture(), material->m_emissiveTextureId, textureId);
			}
		}
	}
	return textureImageInfo;
}

void	VulkanScene::updateGeneralUniformBuffer(uint32_t currentFrame)
{
	//Model View Proj
	GeneralUniformBufferObject ubo{};
	ubo.view = m_camera->getViewMatrix();
	ubo.proj = m_camera->getProjMatrix(m_context);
	ubo.cameraPos = m_camera->getCameraPos();
	ubo.time = m_context->getTime().elapsedSinceStart;
	ubo.shadowMapsBlendWidth = 0.5f;
	ubo.hairLength = 0.03f; // TODO Scene accessible IMGUI stuff
	ubo.gravityFactor = 0.02f;
	ubo.hairDensity = 1000.f;

	//Get the cascade view/proj matrices and frustrum splits previously calculated in the shadowRenderPass
	CascadeUniformObject cascadeUbo = m_cascadeUbos[currentFrame];

	for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
	{
		ubo.cascadeSplits[i] = cascadeUbo.cascadeSplits[i];
		ubo.cascadeViewProj[i] = cascadeUbo.cascadeViewProjMat[i];
	}

	void* data = m_context->getAllocator()->mapMemory(m_generalUniformBuffers[currentFrame].m_Allocation);
	memcpy(data, &ubo, sizeof(GeneralUniformBufferObject));
	m_context->getAllocator()->unmapMemory(m_generalUniformBuffers[currentFrame].m_Allocation);
}

void	VulkanScene::updateUniformBuffers(uint32_t currentFrame)
{
	updateGeneralUniformBuffer(currentFrame);
	updateLightUniformBuffer(currentFrame);
	updateShadowCascadeUniformBuffer(currentFrame);
}

void	VulkanScene::setCamera(Camera* camera)
{
	m_camera = camera;
}

void	VulkanScene::updateShadowCascadeUniformBuffer(uint32_t currentFrame)
{
	//CASCADES
	//Model View Proj
	CascadeUniformObject ubo{};
	//glm::vec3 lightPos = glm::vec3(1.f, 50.f, 2.f);
	//glm::vec3 lightPos = glm::vec3(1.f, 50.f, 20.f * cos(m_context->getTime().elapsedSinceStart/8));
	glm::vec3 lightDirection = m_sun->getWorldDirection();
	float cascadeSplits[SHADOW_CASCADE_COUNT] = { 0.f, 0.f, 0.f, 0.f };

	float nearClip = m_camera->nearPlane;
	float farClip = m_camera->farPlane;
	float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	const float m_cascadeSplitLambda = 0.95f;
	for (uint32_t i = 0; i < SHADOW_CASCADE_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_CASCADE_COUNT);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = m_cascadeSplitLambda * (log - uniform) + uniform;
		cascadeSplits[i] = (d - nearClip) / clipRange;
	}

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t i = 0; i < SHADOW_CASCADE_COUNT; i++) {
		float splitDist = cascadeSplits[i];

		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		glm::mat4 invCam = glm::inverse(m_camera->getProjMatrix(m_context) * m_camera->getViewMatrix());
		for (uint32_t i = 0; i < 8; i++) {
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
			frustumCorners[i] = invCorner / invCorner.w;
		}

		for (uint32_t i = 0; i < 4; i++) {
			glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
			frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
			frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t i = 0; i < 8; i++) {
			frustumCenter += frustumCorners[i];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t i = 0; i < 8; i++) {
			float distance = glm::length(frustumCorners[i] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		const float m_shadowMapsBLendWidth = 0.5f;
		const float higher = m_camera->farPlane * (1 + m_shadowMapsBLendWidth); //Why does this fix everything :sob:. Added blend width to make sure we don't have the boundary of two shadow maps when blending
		glm::vec3 lightDir = normalize(lightDirection);
		glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * (-minExtents.z + higher), frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z + higher);

		// Store split distance and matrix in cascade
		ubo.cascadeSplits[i] = (m_camera->nearPlane + splitDist * clipRange) * -1.0f;
		ubo.cascadeViewProjMat[i] = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = cascadeSplits[i];
	}
	m_cascadeUbos[currentFrame] = ubo;

	void* data = m_context->getAllocator()->mapMemory(m_shadowCascadeUniformBuffers[currentFrame].m_Allocation);
	memcpy(data, &ubo, sizeof(CascadeUniformObject));
	m_context->getAllocator()->unmapMemory(m_shadowCascadeUniformBuffers[currentFrame].m_Allocation);
}

//Updates uniform buffer for Light uniform data
void	VulkanScene::updateLightUniformBuffer(uint32_t currentFrame)
{
	std::vector<Light*> lights = getLights();
	std::array<LightUBO, MAX_LIGHT_COUNT> lightsUbo;
	for (uint32_t i = 0; i < MAX_LIGHT_COUNT; i++)
	{
		if (i < lights.size())
		{
			LightUBO ubo = lights[i]->getUniformData();
			lightsUbo[i] = ubo;
		}
		else {
			lightsUbo[i] = LightUBO{};
		}
	}

	void* data = m_context->getAllocator()->mapMemory(m_lightUniformBuffers[currentFrame].m_Allocation);
	memcpy(data, lightsUbo.data(), sizeof(LightUBO) * lightsUbo.size());
	m_context->getAllocator()->unmapMemory(m_lightUniformBuffers[currentFrame].m_Allocation);
}