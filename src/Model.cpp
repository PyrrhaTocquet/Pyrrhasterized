/*
author: Pyrrha Tocquet
date: 01/06/23
desc: Manages model loading and drawing
*/

#include "Model.h"
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include "tiny_gltf.h"
#include <future>
#include <thread>


Model::Model(VulkanContext* context, const std::filesystem::path& path, const Transform& transform) {
	m_transform = transform;
	m_context = context;

	loadModel(path);
}

Model::~Model() {
	for (auto& textureMesh : m_meshes)
	{
		delete textureMesh.material;
	}
}


//Proxy function used for multithreading
static VulkanImage* newVulkanImage(VulkanContext* context, VulkanImageParams imageParams, VulkanImageViewParams imageViewParams, std::string path)
{
	return new VulkanImage(context, imageParams, imageViewParams, path);
}

static void createAlbedoTextureFromGltfMaterial(VulkanContext* context, Mesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path)
{
	int gltfTextureId = materialInfo.pbrMetallicRoughness.baseColorTexture.index;
	if (gltfTextureId == -1)
		return;

	std::string texturePath = gltfModel.images[gltfModel.textures[gltfTextureId].source].uri;
	if (texturePath == "")texturePath = gltfModel.images[gltfModel.textures[gltfTextureId].source].name + ".png";
	if (texturePath == "")
		return;

	if (texturePath != "")
	{
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

		VulkanImage* image = newVulkanImage(context, imageParams, imageViewParams, path.string() + "/" + texturePath);
		if (image->hasLoadingFailed())
		{
			delete image;
			return;
		}
		texturedMesh.material->setAlbedoTexture(image);
	}

}

static void createNormalTextureFromGltfMaterial(VulkanContext* context, Mesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path)
{
	int gltfTextureId = materialInfo.normalTexture.index;
	if (gltfTextureId == -1)
		return;

	std::string texturePath = gltfModel.images[gltfModel.textures[gltfTextureId].source].uri;
	if (texturePath == "")texturePath = gltfModel.images[gltfModel.textures[gltfTextureId].source].name + ".png";
	if (texturePath == "")
		return;

	if (texturePath != "")
	{
		VulkanImageParams imageParams
		{
			.numSamples = vk::SampleCountFlagBits::e1,
			.format = vk::Format::eR8G8B8A8Unorm,
			.tiling = vk::ImageTiling::eOptimal,
			.usage = vk::ImageUsageFlagBits::eSampled,
		};
		VulkanImageViewParams imageViewParams{
			.aspectFlags = vk::ImageAspectFlagBits::eColor,
		};
		
		texturedMesh.material->setNormalTexture(newVulkanImage(context, imageParams, imageViewParams, path.string() + "/" + texturePath));
	}
}

static void createMetallicRoughnessTexture(VulkanContext* context, Mesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path)
{
	int gltfTextureId = materialInfo.pbrMetallicRoughness.metallicRoughnessTexture.index;
	if (gltfTextureId == -1)
		return;

	std::string texturePath = gltfModel.images[gltfModel.textures[gltfTextureId].source].uri;
	if (texturePath == "")texturePath = gltfModel.images[gltfModel.textures[gltfTextureId].source].name + ".png";
	if (texturePath == "")
		return;

	if (texturePath != "")
	{
		VulkanImageParams imageParams
		{
			.numSamples = vk::SampleCountFlagBits::e1,
			.format = vk::Format::eR8G8B8A8Unorm,
			.tiling = vk::ImageTiling::eOptimal,
			.usage = vk::ImageUsageFlagBits::eSampled,
		};
		VulkanImageViewParams imageViewParams{
			.aspectFlags = vk::ImageAspectFlagBits::eColor,
		};
		texturedMesh.material->setMetallicRoughnessTexture(newVulkanImage(context, imageParams, imageViewParams, path.string() + "/" + texturePath));
	}

}

static void createEmissiveTexture(VulkanContext* context, Mesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path)
{
	int gltfTextureId = materialInfo.emissiveTexture.index;
	if (gltfTextureId == -1)
		return;

	std::string texturePath = gltfModel.images[gltfModel.textures[gltfTextureId].source].uri;
	if (texturePath == "")texturePath = gltfModel.images[gltfModel.textures[gltfTextureId].source].name + ".png";
	if (texturePath == "")
		return;

	if (texturePath != "")
	{
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
		texturedMesh.material->setEmissiveTexture(newVulkanImage(context, imageParams, imageViewParams, path.string() + "/" + texturePath));
	}

}

//Helper function to load gltf material data into a TexturedMesh material data, if a material already exists, it will not be replaced
static void createMaterialFromGltf(VulkanContext* context, Mesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path){
	if(texturedMesh.material == nullptr)
	{
		std::filesystem::path parentPath = path.parent_path();
		std::vector<std::future<VulkanImage*>> textureLoadFutures;

		glm::vec4 baseColor = glm::vec4(materialInfo.pbrMetallicRoughness.baseColorFactor[0], materialInfo.pbrMetallicRoughness.baseColorFactor[1], materialInfo.pbrMetallicRoughness.baseColorFactor[2], materialInfo.pbrMetallicRoughness.baseColorFactor[3]);
		glm::vec4 emissiveFactor = glm::vec4(materialInfo.emissiveFactor[0], materialInfo.emissiveFactor[1], materialInfo.emissiveFactor[2], 1.f);
		texturedMesh.material = (new Material(context))
			->setBaseColor(baseColor)
			->setMetallicFactor(materialInfo.pbrMetallicRoughness.metallicFactor)
			->setEmissiveFactor(emissiveFactor)
			->setRoughnessFactor(materialInfo.pbrMetallicRoughness.roughnessFactor);

		texturedMesh.material->setAlphaCutoff(materialInfo.alphaCutoff);
		if (materialInfo.alphaMode == "MASK")
		{
			texturedMesh.material->setAlphaMode(MaskAlphaMode);
		}
		else if (materialInfo.alphaMode == "BLEND") {
			texturedMesh.material->setAlphaMode(TransparentAlphaMode);
		}

		createAlbedoTextureFromGltfMaterial(context, texturedMesh, materialInfo, gltfModel, parentPath);
		createNormalTextureFromGltfMaterial(context, texturedMesh, materialInfo, gltfModel, parentPath);
		createMetallicRoughnessTexture(context, texturedMesh, materialInfo, gltfModel, parentPath);
		createEmissiveTexture(context, texturedMesh, materialInfo, gltfModel, parentPath);
	}

}

//returns the model matrix (world space) of the model computed from transform data
glm::mat4 Model::getMatrix()
{
	return m_transform.computeMatrix();
}

//Write command buffer at scene drawing
void Model::drawModel(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t &indexOffset, ModelPushConstant& pushConstant)
{

	for (auto& mesh : m_meshes)
	{
		pushConstant.model = m_transform.computeMatrix();
		pushConstant.materialId = static_cast<glm::int32>(mesh.materialId);

		commandBuffer.pushConstants<ModelPushConstant>(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
		commandBuffer.drawIndexed(mesh.indicesCount, 1, indexOffset, 0, 0);
		indexOffset += mesh.indicesCount;
	}
}

//returns textured meshes dividing the model
std::vector<Mesh>& Model::getMeshes()
{
	return m_meshes;
}

//Translates the model by the inputed vector (do before computing model matrix)
void Model::translateBy(glm::vec3 translation)
{
	m_transform.translate += translation;
	m_transform.hasChanged = true;
}

//Rotates the model by the inputed vector (do before computing model matrix). Rotations are in degrees.
void Model::rotateBy(glm::vec3 rotation)
{
	m_transform.rotate += rotation;
	m_transform.hasChanged = true;
}

//Scales the model by the inputed vector (do before computing model matrix)
void Model::scaleBy(glm::vec3 scale)
{
	m_transform.scale *= scale;
	m_transform.hasChanged = true;
}

//Releases vertices memory
void Model::clearLoadingVertexData()
{
	for (auto& mesh : m_meshes)
	{
		mesh.verticesCount = mesh.loadingVertices.size();
		std::vector<Vertex>().swap(mesh.loadingVertices);

	}
}

//Releases indices memory
void Model::clearLoadingIndexData()
{
	for (auto& mesh : m_meshes)
	{
		mesh.indicesCount = mesh.loadingIndices.size();
		std::vector<uint32_t>().swap(mesh.loadingIndices);
	}
}



//Picks the right tinygltf function depending on the file format and manages errors
static void loadGltfData(const std::filesystem::path& path, tinygltf::Model& gltfModel) {
	bool ret = false;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

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

	if (!err.empty()) {
		throw std::runtime_error(err);
	}

	if (!ret) {
		throw std::runtime_error("Failed to parse glTf\n");
	}

}

//Loads GLTF and GLB files
void Model::loadGltf(const std::filesystem::path& path)
{
	tinygltf::Model gltfModel;
	loadGltfData(path, gltfModel);
	
	size_t materialCount = gltfModel.materials.size();
	m_meshes.resize(materialCount);

	if (materialCount <= 0)
	{
		Mesh texturedMesh;
		m_meshes.push_back(texturedMesh);
	}
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
				if (materialCount <= 0)
				{
					materialIndex = 0;
				}
				m_meshes[materialIndex].loadingVertices.push_back(vertex);
				m_meshes[materialIndex].loadingIndices.push_back(m_meshes[materialIndex].loadingIndices.size());

			}

		}
	}
	std::vector<std::jthread> materialsFromGltfThread;
	materialsFromGltfThread.resize(materialCount);
	for (uint32_t materialIndex = 0; materialIndex < materialCount; materialIndex++)
	{
		materialsFromGltfThread[materialIndex] = std::jthread(createMaterialFromGltf, m_context, std::ref(m_meshes[materialIndex]), std::ref(gltfModel.materials[materialIndex]), std::ref(gltfModel), std::ref(path));
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

//Used in a new thread by generateTangents to compute tangent data
static void generateTangentData(Mesh* texturedMesh) {
	for (uint32_t i = 0; i < texturedMesh->loadingIndices.size(); i += 3)
	{
		uint32_t i0 = texturedMesh->loadingIndices[i + 0];
		uint32_t i1 = texturedMesh->loadingIndices[i + 1];
		uint32_t i2 = texturedMesh->loadingIndices[i + 2];

		glm::vec3 edge1 = texturedMesh->loadingVertices[i1].pos - texturedMesh->loadingVertices[i0].pos;
		glm::vec3 edge2 = texturedMesh->loadingVertices[i2].pos - texturedMesh->loadingVertices[i0].pos;

		glm::vec2 deltaUV1 = texturedMesh->loadingVertices[i1].texCoord - texturedMesh->loadingVertices[i0].texCoord;
		glm::vec2 deltaUV2 = texturedMesh->loadingVertices[i2].texCoord - texturedMesh->loadingVertices[i0].texCoord;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

		glm::vec3 tangent3 = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r;
		tangent3 = glm::normalize(tangent3);
		float handedness = ((deltaUV1.y * deltaUV2.x - deltaUV2.y * deltaUV1.x) < 0.0f) ? -1.0f : 1.0f;

		glm::vec4 tangent4 = glm::vec4(tangent3, handedness);
		texturedMesh->loadingVertices[i0].tangent = tangent4;
		texturedMesh->loadingVertices[i1].tangent = tangent4;
		texturedMesh->loadingVertices[i2].tangent = tangent4;
	}
}
//Generates the tangent data in the Vertex struct
void Model::generateTangents() {
	std::vector<std::jthread> generateTangentDataThreads;
	generateTangentDataThreads.resize(m_meshes.size());
	for (uint32_t i = 0; i < m_meshes.size(); i++)
	{
		generateTangentDataThreads[i] = std::jthread(generateTangentData, &m_meshes[i]);
	}
}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	uint32_t materialCount = materials.size();
	if (materialCount == 0) {
		materialCount = 1;
	}

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
