#include "VulkanScene.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"


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
		for (const auto& mesh : model.texturedMeshes) {
			delete mesh.textureImage;
			delete mesh.normalMapImage;
		}
	}

}

void VulkanScene::addChildren(VulkanScene* childrenScene) {
	if (childrenScene == nullptr) {
		throw std::runtime_error("Children scene is nullptr !");
	}
	m_childrenScenes.push_back(childrenScene);
}

void VulkanScene::addObjModel(const std::string& path, const glm::mat4& modelMatrix) 
{
	Model model;
	model.matrix = modelMatrix;
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = ""; // Path to material files

	tinyobj::ObjReader reader;


	if (!reader.ParseFromFile(path + "/model.obj", reader_config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();
	
	model.texturedMeshes.resize(materials.size());

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				Vertex vertex;
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				vertex.pos = {
					attrib.vertices[3 * size_t(idx.vertex_index) + 0],
					attrib.vertices[3 * size_t(idx.vertex_index) + 1],
					attrib.vertices[3 * size_t(idx.vertex_index) + 2],
				};

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * size_t(idx.normal_index) + 0],
						attrib.normals[3 * size_t(idx.normal_index) + 1],
						attrib.normals[3 * size_t(idx.normal_index) + 2],
					};
				}
				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					vertex.texCoord =
					{
						attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
						1- attrib.texcoords[2 * size_t(idx.texcoord_index) + 1],
					};

				}
				int materialIndex = shapes[s].mesh.material_ids[f];
				model.texturedMeshes[materialIndex].vertices.push_back(vertex);
				model.texturedMeshes[materialIndex].indices.push_back(model.texturedMeshes[materialIndex].indices.size());
			}
			index_offset += fv;
			
		}
	}
	//Creating Textures
	VulkanImageParams imageParams
	{
		.numSamples = vk::SampleCountFlagBits::e1,
		.format = vk::Format::eR8G8B8A8Srgb,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eSampled,
	};

	VulkanImageViewParams imageViewParams{
		.aspectFlags = vk::ImageAspectFlagBits::eColor,
	};
	for (int i = 0; i < model.texturedMeshes.size(); i++) {
		if (materials[i].diffuse_texname != "")
		{
			model.texturedMeshes[i].textureImage = new VulkanImage(m_context, imageParams, imageViewParams, path + "/" + materials[i].diffuse_texname);
		}
	}

	//Creating NormalMaps
	VulkanImageParams normalImageParams
	{
		.numSamples = vk::SampleCountFlagBits::e1,
		.format = vk::Format::eR8G8B8A8Unorm,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eSampled,
	};

	VulkanImageViewParams normalImageViewParams{
		.aspectFlags = vk::ImageAspectFlagBits::eColor,
	};
	for (int i = 0; i < model.texturedMeshes.size(); i++) {
		if (materials[i].diffuse_texname != "")
		{
			if (materials[i].displacement_texname != "")
			{
				model.texturedMeshes[i].normalMapImage = new VulkanImage(m_context, normalImageParams, normalImageViewParams, path + "/" + materials[i].displacement_texname);
			}
			
		}
	}
	generateTangents(model);
	m_models.push_back(model);

}


void VulkanScene::addGltfModel(const std::filesystem::path& path, const glm::mat4& modelMatrix)
{
	Model model;
	model.matrix = modelMatrix;
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = false;
	std::filesystem::path ext = path.extension();
	if (path.extension() == ".gltf")
	{
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path.string());
	}
	else if (path.extension() == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path.string());
	}
	else {
		std::runtime_error("Model format not supported");
	}
	
	/*
	if (!warn.empty()) {

	}*/
	if (!err.empty()) {
		throw std::runtime_error(err);
	}

	if (!ret) {
		throw std::runtime_error("Failed to parse glTf\n");
	}
	model.texturedMeshes.resize(gltfModel.materials.size());
	for (const auto& mesh : gltfModel.meshes) {
		for (const auto& attribute : mesh.primitives) {
			int positionBufferIndex = attribute.attributes.at("POSITION");
			int texCoordBufferIndex = attribute.attributes.at("TEXCOORD_0");
			int normalBufferIndex = attribute.attributes.at("NORMAL");
			int indicesBufferIndex = attribute.indices;

			//Vertices
			for (int i = 0; i < gltfModel.accessors[indicesBufferIndex].count; i++) {
				int positionBufferOffset = gltfModel.accessors[positionBufferIndex].byteOffset + gltfModel.bufferViews[gltfModel.accessors[positionBufferIndex].bufferView].byteOffset;
				int texCoordBufferOffset = gltfModel.accessors[texCoordBufferIndex].byteOffset + gltfModel.bufferViews[gltfModel.accessors[texCoordBufferIndex].bufferView].byteOffset;
				int indicesBufferOffset = gltfModel.accessors[indicesBufferIndex].byteOffset + gltfModel.bufferViews[gltfModel.accessors[indicesBufferIndex].bufferView].byteOffset;
				int normalBufferOffset = gltfModel.accessors[normalBufferIndex].byteOffset + gltfModel.bufferViews[gltfModel.accessors[normalBufferIndex].bufferView].byteOffset;

				int positionBufferStride = gltfModel.bufferViews[gltfModel.accessors[positionBufferIndex].bufferView].byteStride;
				int texCoordBufferStride = gltfModel.bufferViews[gltfModel.accessors[texCoordBufferIndex].bufferView].byteStride;
				int normalBufferStride = gltfModel.bufferViews[gltfModel.accessors[normalBufferIndex].bufferView].byteStride;

				if (positionBufferStride == 0)positionBufferStride = 4 * 3;
				if (texCoordBufferStride == 0)texCoordBufferStride = 4 * 2;
				if (normalBufferStride == 0)normalBufferStride = 4 * 3;

				uint16_t index = *(uint16_t*)(&gltfModel.buffers[0].data[indicesBufferOffset + 2 * i]); //TODO better index type handling

				Vertex vertex{};
				if (positionBufferStride * index + 2 * 4 > gltfModel.bufferViews[gltfModel.accessors[positionBufferIndex].bufferView].byteLength)
				{
					std::cout << "position exceeded !" << std::endl;
				}
				vertex.pos = glm::vec3(*(float*)(&gltfModel.buffers[0].data[positionBufferOffset + positionBufferStride * index + 0 * 4]),
					*(float*)(&gltfModel.buffers[0].data[positionBufferOffset + positionBufferStride * index + 1 * 4]),
					*(float*)(&gltfModel.buffers[0].data[positionBufferOffset + positionBufferStride * index + 2 * 4]));

				vertex.texCoord = {
					*(float*)(&gltfModel.buffers[0].data[texCoordBufferOffset + texCoordBufferStride * index + 0 * 4]),
					*(float*)(&gltfModel.buffers[0].data[texCoordBufferOffset + texCoordBufferStride * index + 1 * 4])
				};

				vertex.normal = {
					*(float*)(&gltfModel.buffers[0].data[normalBufferOffset + normalBufferStride * index + 0 * 4]),
					*(float*)(&gltfModel.buffers[0].data[normalBufferOffset + normalBufferStride * index + 1 * 4]),
					*(float*)(&gltfModel.buffers[0].data[normalBufferOffset + normalBufferStride * index + 2 * 4])
				};

				int materialIndex = attribute.material;
				model.texturedMeshes[materialIndex].vertices.push_back(vertex);
				model.texturedMeshes[materialIndex].indices.push_back(model.texturedMeshes[materialIndex].indices.size());
			}

		}
	}

	//Creating Textures
	std::filesystem::path parentPath = path.parent_path();
	VulkanImageParams imageParams
	{
		.numSamples = vk::SampleCountFlagBits::e1,
		.format = vk::Format::eR8G8B8A8Srgb,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eSampled,
	};

	VulkanImageViewParams imageViewParams{
		.aspectFlags = vk::ImageAspectFlagBits::eColor,
	};
	for (int i = 0; i < model.texturedMeshes.size(); i++) {
		int textureId = gltfModel.materials[i].pbrMetallicRoughness.baseColorTexture.index;
		if (textureId != -1)
		{
			std::string texturePath = gltfModel.images[gltfModel.textures[textureId].source].uri;
			model.texturedMeshes[i].textureImage = new VulkanImage(m_context, imageParams, imageViewParams, parentPath.string() + "/" + texturePath);
		}
	}

	//Creating NormalMaps
	VulkanImageParams normalImageParams
	{
		.numSamples = vk::SampleCountFlagBits::e1,
		.format = vk::Format::eR8G8B8A8Unorm,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eSampled,
	};

	VulkanImageViewParams normalImageViewParams{
		.aspectFlags = vk::ImageAspectFlagBits::eColor,
	};
	for (int i = 0; i < model.texturedMeshes.size(); i++) {
		int textureId = gltfModel.materials[i].normalTexture.index;
		if (textureId != -1)
		{
			std::string texturePath = gltfModel.images[gltfModel.textures[textureId].source].uri;
			model.texturedMeshes[i].normalMapImage = new VulkanImage(m_context, normalImageParams, normalImageViewParams, parentPath.string() + "/" + texturePath);
		}
	}
	generateTangents(model);
	m_models.push_back(model);

}

void VulkanScene::createBuffers()
{
	createVertexBuffer();
	createIndexBuffer();
}

void VulkanScene::setDescriptorSets(std::vector<vk::DescriptorSet> descriptorSet)
{
	m_descriptorSets = descriptorSet;
}

const uint32_t VulkanScene::getIndexBufferSize()
{
	uint32_t indicesCount = 0;
	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model.texturedMeshes) {
			indicesCount += texturedMesh.indices.size();
		}

	}
	return indicesCount;
}

void VulkanScene::draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout)
{
	static auto startTime = std::chrono::high_resolution_clock::now(); //Todo, engine solution for time

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - startTime).count();

	VkDeviceSize offset = 0;
	commandBuffer.bindVertexBuffers(0, 1, &m_vertexBuffer, &offset);
	commandBuffer.bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint32);

	uint32_t indexOffset = 0;
	//Draws each model in a scene
	for (auto& model : m_models) {
		//Draws each mesh with a unique texture
		for (auto& mesh : model.texturedMeshes)
		{
			
			ModelPushConstant modelPushConstant{
				.model = model.matrix,
				.textureId = static_cast<glm::int32>(mesh.textureId),
				.normalMapId = static_cast<glm::int32>(mesh.normalMapId),
				.time = time,
			};

			commandBuffer.pushConstants<ModelPushConstant>(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, modelPushConstant);
			commandBuffer.drawIndexed(mesh.indices.size(), 1, indexOffset, 0, 0);
			indexOffset += mesh.indices.size();
		}
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
		for (int texturedMeshIndex = 0; texturedMeshIndex < m_models[modelIndex].texturedMeshes.size(); texturedMeshIndex++) {
			
			auto& texturedMesh =  m_models[modelIndex].texturedMeshes[texturedMeshIndex];
			for (auto& index : texturedMesh.indices) {
				index += indexOffset;
			}
			indexOffset += texturedMesh.indices.size();
			memcpy(data, texturedMesh.indices.data(), static_cast<size_t>(texturedMesh.indices.size()*sizeof(uint32_t)));
			data += texturedMesh.indices.size() * sizeof(uint32_t);
		}
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
		for (const auto& texturedMesh : model.texturedMeshes) {
			verticesCount += texturedMesh.vertices.size();
		}
		
	}

	vk::DeviceSize bufferSize = sizeof(Vertex) * verticesCount;

	vk::Buffer stagingBuffer;
	vma::Allocation stagingAllocation;
	std::tie(stagingBuffer, stagingAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vma::MemoryUsage::eCpuToGpu);

	char* data = static_cast<char*>(m_allocator.mapMemory(stagingAllocation));

	for (const auto& model : m_models) {
		for (const auto& texturedMesh : model.texturedMeshes) {
			memcpy(data, texturedMesh.vertices.data(), static_cast<size_t>(texturedMesh.vertices.size() * sizeof(Vertex)));
			data += texturedMesh.vertices.size() * sizeof(Vertex);
		}
	}
	m_allocator.unmapMemory(stagingAllocation);

	std::tie(m_vertexBuffer, m_vertexBufferAllocation) = m_context->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vma::MemoryUsage::eGpuOnly);

	m_context->copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	m_allocator.destroyBuffer(stagingBuffer, stagingAllocation);
}

//Generates tangent data in the model vertices
void VulkanScene::generateTangents(Model& model)
{
	for (auto& texturedMesh : model.texturedMeshes)
	{
		for (uint32_t i = 0; i < texturedMesh.indices.size(); i += 3)
		{
			uint32_t i0 = texturedMesh.indices[i + 0];
			uint32_t i1 = texturedMesh.indices[i + 1];
			uint32_t i2 = texturedMesh.indices[i + 2];

			glm::vec3 edge1 = texturedMesh.vertices[i1].pos - texturedMesh.vertices[i0].pos;
			glm::vec3 edge2 = texturedMesh.vertices[i2].pos - texturedMesh.vertices[i0].pos;

			glm::vec2 deltaUV1 = texturedMesh.vertices[i1].texCoord - texturedMesh.vertices[i0].texCoord;
			glm::vec2 deltaUV2 = texturedMesh.vertices[i2].texCoord - texturedMesh.vertices[i0].texCoord;

			float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

			glm::vec3 tangent3 = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r;
			tangent3 = glm::normalize(tangent3);
			float handedness = ((deltaUV1.y * deltaUV2.x - deltaUV2.y * deltaUV1.x) < 0.0f) ? -1.0f : 1.0f;

			glm::vec4 tangent4 = glm::vec4(tangent3, handedness);
			texturedMesh.vertices[i0].tangent = tangent4;
			texturedMesh.vertices[i1].tangent = tangent4;
			texturedMesh.vertices[i2].tangent = tangent4;
		}
	}
}