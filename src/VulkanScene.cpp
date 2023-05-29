#include "VulkanScene.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
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