#include "Model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"


Model::Model(VulkanContext* context, const std::filesystem::path& path, const Transform& transform) {
	m_transform = transform;
	m_context = context;

	loadModel(path);
}

Model::~Model() {
	for (auto& textureMesh : m_texturedMeshes)
	{
		delete textureMesh.textureImage;
		delete textureMesh.normalMapImage;
	}
}

glm::mat4 Model::getMatrix()
{
	return m_transform.computeMatrix();
}

//Write command buffer at scene drawing
void Model::drawModel(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, float time, uint32_t &indexOffset)
{

	for (auto& mesh : m_texturedMeshes)
	{

		ModelPushConstant modelPushConstant{
			.model = m_transform.computeMatrix(),
			.textureId = static_cast<glm::int32>(mesh.textureId),
			.normalMapId = static_cast<glm::int32>(mesh.normalMapId),
			.time = time,
		};

		commandBuffer.pushConstants<ModelPushConstant>(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, modelPushConstant);
		commandBuffer.drawIndexed(mesh.indices.size(), 1, indexOffset, 0, 0);
		indexOffset += mesh.indices.size();
	}
}

std::vector<TexturedMesh>& Model::getMeshes()
{
	return m_texturedMeshes;
}

void Model::translateBy(glm::vec3 translation)
{
	m_transform.translate += translation;
}

void Model::rotateBy(glm::vec3 rotation)
{
	m_transform.rotate += rotation;
}

void Model::scaleBy(glm::vec3 scale)
{
	m_transform.scale += scale;
}

void Model::loadGltf(const std::filesystem::path& path)
{
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

	m_texturedMeshes.resize(gltfModel.materials.size());
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
				m_texturedMeshes[materialIndex].vertices.push_back(vertex);
				m_texturedMeshes[materialIndex].indices.push_back(m_texturedMeshes[materialIndex].indices.size());
			}

		}
	}
	//TODO Refactor
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
	
	for (int i = 0; i < m_texturedMeshes.size(); i++) {
		int textureId = gltfModel.materials[i].pbrMetallicRoughness.baseColorTexture.index;
		if (textureId != -1)
		{
			std::string texturePath = gltfModel.images[gltfModel.textures[textureId].source].uri;
			m_texturedMeshes[i].textureImage = new VulkanImage(m_context, imageParams, imageViewParams, parentPath.string() + "/" + texturePath);
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
	for (int i = 0; i < m_texturedMeshes.size(); i++) {
		int textureId = gltfModel.materials[i].normalTexture.index;
		if (textureId != -1)
		{
			std::string texturePath = gltfModel.images[gltfModel.textures[textureId].source].uri;
			m_texturedMeshes[i].normalMapImage = new VulkanImage(m_context, normalImageParams, normalImageViewParams, parentPath.string() + "/" + texturePath);
		}
	}
	generateTangents();
};

//Calls the appropriate loading function depending on the file extension
void Model::loadModel(const std::filesystem::path& path) {
	std::filesystem::path extension = path.extension();
	if (extension == ".obj")
	{
		loadObj(path);
	}
	else if (extension == ".gltf" || extension == ".glb") {
		loadGltf(path);
	}
	else {
		std::runtime_error("Only .obj, .gltf and .glb files are supported for 3D model loading");
	}
}

//Generates the tangent data in the Vertex struct
void Model::generateTangents() {
	for (auto& texturedMesh : m_texturedMeshes)
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

void Model::loadObj(const std::filesystem::path& path) {
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = ""; // Path to material files

	tinyobj::ObjReader reader;


	if (!reader.ParseFromFile(path.string(), reader_config)) {
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

	m_texturedMeshes.resize(materials.size());

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
						1 - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1],
					};

				}
				int materialIndex = shapes[s].mesh.material_ids[f];
				m_texturedMeshes[materialIndex].vertices.push_back(vertex);
				m_texturedMeshes[materialIndex].indices.push_back(m_texturedMeshes[materialIndex].indices.size());
			}
			index_offset += fv;

		}
	}
	std::filesystem::path parentPath = path.parent_path();
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
	for (int i = 0; i < m_texturedMeshes.size(); i++) {
		if (materials[i].diffuse_texname != "")
		{
			m_texturedMeshes[i].textureImage = new VulkanImage(m_context, imageParams, imageViewParams, parentPath.string() + "/" + materials[i].diffuse_texname);
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
	for (int i = 0; i < m_texturedMeshes.size(); i++) {
		if (materials[i].diffuse_texname != "")
		{
			if (materials[i].displacement_texname != "")
			{
				m_texturedMeshes[i].normalMapImage = new VulkanImage(m_context, normalImageParams, normalImageViewParams, parentPath.string() + "/" + materials[i].displacement_texname);
			}

		}
	}
	generateTangents();
}