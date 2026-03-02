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


Model::Model(VulkanContext* context, std::filesystem::path path)
	: 	m_context{context}
	,	m_path{path}
	,	m_rawMeshes{}
	,	m_meshes{}
	,	m_isLoaded{false}
{
	//TODO Dynamic loading
	vkDrawMeshTasks = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(m_context->getDevice(), "vkCmdDrawMeshTasksEXT");
}

Model::~Model() {
	for (auto& textureMesh : m_rawMeshes)
	{
		delete textureMesh.material;
	}
}


//Proxy function used for multithreading
static VulkanImage* newVulkanImage(VulkanContext* context, VulkanImageParams imageParams, VulkanImageViewParams imageViewParams, std::string path)
{
	return new VulkanImage(context, imageParams, imageViewParams, path);
}

static void createAlbedoTextureFromGltfMaterial(VulkanContext* context, RawMesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path)
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

static void createNormalTextureFromGltfMaterial(VulkanContext* context, RawMesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path)
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

static void createMetallicRoughnessTexture(VulkanContext* context, RawMesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path)
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

static void createEmissiveTexture(VulkanContext* context, RawMesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path)
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
static void createMaterialFromGltf(VulkanContext* context, RawMesh& texturedMesh, const tinygltf::Material& materialInfo, const tinygltf::Model& gltfModel, const std::filesystem::path& path){
	if(texturedMesh.material == nullptr)
	{
		std::filesystem::path parentPath = path.parent_path();
		std::vector<std::future<VulkanImage*>> textureLoadFutures;

		glm::vec4 baseColor = glm::vec4(materialInfo.pbrMetallicRoughness.baseColorFactor[0], materialInfo.pbrMetallicRoughness.baseColorFactor[1], materialInfo.pbrMetallicRoughness.baseColorFactor[2], materialInfo.pbrMetallicRoughness.baseColorFactor[3]);
		glm::vec4 emissiveFactor = glm::vec4(materialInfo.emissiveFactor[0], materialInfo.emissiveFactor[1], materialInfo.emissiveFactor[2], 1.f);
		texturedMesh.material = (new Material(context))
			->setBaseColor(baseColor)
			->setMetallicFactor(static_cast<float>(materialInfo.pbrMetallicRoughness.metallicFactor))
			->setEmissiveFactor(emissiveFactor)
			->setRoughnessFactor(static_cast<float>(materialInfo.pbrMetallicRoughness.roughnessFactor));

		texturedMesh.material->setAlphaCutoff(static_cast<float>(materialInfo.alphaCutoff));
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

//Write command buffer at scene drawing
void Model::drawModel(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant)
{
	uint32_t k = 0;
	for (int i = 0; i < m_rawMeshes.size(); i++)
	{
		pushConstant.materialId = static_cast<glm::int32>(m_rawMeshes[i].materialId);
		
		if(m_meshes[i].meshlets.size() > 0)
		{
			pushConstant.meshlet = m_meshes[i].meshlets[0].meshletInfo.meshletId;
			pushConstant.meshletCount = static_cast<glm::uint32>(m_meshes[i].meshlets.size());
			commandBuffer.pushConstants<ModelPushConstant>(pipelineLayout, vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);

			vkDrawMeshTasks(commandBuffer, static_cast<uint32_t>(m_meshes[i].meshlets.size()), 1, 1);
		}
			
		k += static_cast<uint32_t>(m_meshes[i].meshlets.size());
	}
}

//returns textured meshes dividing the model
std::vector<Mesh>& Model::getMeshes()
{
	return m_meshes;
}

//returns textured meshes dividing the model
std::vector<RawMesh>& Model::getRawMeshes()
{
	return m_rawMeshes;
}


//Releases vertices memory
void Model::clearLoadingVertexData()
{
	for (auto& mesh : m_rawMeshes)
	{
		mesh.verticesCount = static_cast<uint32_t>(mesh.loadingVertices.size());
		std::vector<Vertex>().swap(mesh.loadingVertices);

	}
}

//Releases indices memory
void Model::clearLoadingIndexData()
{
	for (auto& mesh : m_rawMeshes)
	{
		mesh.indicesCount = static_cast<uint32_t>(mesh.loadingIndices.size());
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
		throw std::runtime_error("Model format not supported");
	}

	if (!err.empty()) {
		throw std::runtime_error(err);
	}

	if (!ret) {
		throw std::runtime_error("Failed to parse glTf\n");
	}

}

//Loads GLTF and GLB files
void Model::loadGltf(const std::filesystem::path& path, bool isBaked)
{
	tinygltf::Model gltfModel;
	loadGltfData(path, gltfModel);
	
	size_t materialCount = gltfModel.materials.size();
	m_rawMeshes.resize(materialCount);

	if (materialCount <= 0)
	{
		RawMesh texturedMesh;
		m_rawMeshes.push_back(texturedMesh);
	}

	if (!isBaked)
	{
		for (const auto& mesh : gltfModel.meshes) 
		{
			for (const auto& attribute : mesh.primitives) 
			{
				size_t positionBufferIndex = attribute.attributes.at("POSITION");
				size_t texCoordBufferIndex = attribute.attributes.at("TEXCOORD_0");
				size_t normalBufferIndex = attribute.attributes.at("NORMAL");
				size_t indicesBufferIndex = attribute.indices;

				//Vertices
				for (size_t i = 0; i < gltfModel.accessors[indicesBufferIndex].count; i++)
				{
					size_t positionBufferOffset = gltfModel.accessors[positionBufferIndex].byteOffset + gltfModel.bufferViews[gltfModel.accessors[positionBufferIndex].bufferView].byteOffset;
					size_t texCoordBufferOffset = gltfModel.accessors[texCoordBufferIndex].byteOffset + gltfModel.bufferViews[gltfModel.accessors[texCoordBufferIndex].bufferView].byteOffset;
					size_t indicesBufferOffset = gltfModel.accessors[indicesBufferIndex].byteOffset + gltfModel.bufferViews[gltfModel.accessors[indicesBufferIndex].bufferView].byteOffset;
					size_t normalBufferOffset = gltfModel.accessors[normalBufferIndex].byteOffset + gltfModel.bufferViews[gltfModel.accessors[normalBufferIndex].bufferView].byteOffset;

					size_t positionBufferStride = gltfModel.bufferViews[gltfModel.accessors[positionBufferIndex].bufferView].byteStride;
					size_t texCoordBufferStride = gltfModel.bufferViews[gltfModel.accessors[texCoordBufferIndex].bufferView].byteStride;
					size_t normalBufferStride = gltfModel.bufferViews[gltfModel.accessors[normalBufferIndex].bufferView].byteStride;

					if (positionBufferStride == 0)positionBufferStride = size_t(4 * 3);
					if (texCoordBufferStride == 0)texCoordBufferStride =  size_t(4 * 2);
					if (normalBufferStride == 0)normalBufferStride =  size_t(4 * 3);

					size_t index = *(uint16_t*)(&gltfModel.buffers[0].data[indicesBufferOffset + 2 * i]); //TODO better index type handling

					Vertex vertex{};
					if (positionBufferStride * index + size_t(2 * 4) > gltfModel.bufferViews[gltfModel.accessors[positionBufferIndex].bufferView].byteLength)
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
					m_rawMeshes[materialIndex].loadingVertices.push_back(vertex);
					m_rawMeshes[materialIndex].loadingIndices.push_back(static_cast<uint32_t>(m_rawMeshes[materialIndex].loadingIndices.size()));

				}
			}
		}
	}
	
	for (uint32_t materialIndex = 0; materialIndex < materialCount; materialIndex++)
	{
		createMaterialFromGltf(m_context, m_rawMeshes[materialIndex], gltfModel.materials[materialIndex], gltfModel, path);
	}

	if (!isBaked)
		generateTangents();
		

};

//Calls the appropriate loading function depending on the file extension
void Model::loadModel() {
	
	if (m_isLoaded)
		return;

	const bool isBaked = SerializationTools::isModelBaked(m_path);

	//Load GLTF
	std::filesystem::path extension = m_path.extension();
	if (extension == ".gltf" || extension == ".glb") {
		loadGltf(m_path, isBaked);
	}
	else {
		throw std::runtime_error("Only .gltf and .glb files are supported for 3D model loading/baking");
	}

	// Serialization has not been a success lol
	if(!isBaked)
	{
		//Bake meshlets
		for(RawMesh &mesh: m_rawMeshes)
		{
			uint32_t maxPrimitives = 128;
			uint32_t maxVertices = 128;

			m_meshes.emplace_back();
			if(mesh.loadingIndices.size() > 0)
			{
				GeometryTools::bakeMeshlets(maxPrimitives, maxVertices, mesh.loadingIndices.data(), static_cast<uint32_t>(mesh.loadingIndices.size()), mesh.loadingVertices, m_meshes.back().meshlets);
				m_meshes.back().vertices = mesh.loadingVertices;
				
			}
		}

		std::vector<std::jthread> writeBakedModelThreads;
		writeBakedModelThreads.resize(m_meshes.size());
	
		SerializationTools::writeBakedModel(m_path, m_meshes);     
	}
	else
	{
		SerializationTools::loadBakedModel(m_path, m_meshes);
		std::cout << "Loaded Model: " << m_path << std::endl;
	}
	m_isLoaded = true;
}

//Used in a new thread by generateTangents to compute tangent data
static void generateTangentData(RawMesh* texturedMesh) {
	for (size_t i = 0; i < texturedMesh->loadingIndices.size(); i += 3)
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
	generateTangentDataThreads.resize(m_rawMeshes.size());
	for (uint32_t i = 0; i < m_rawMeshes.size(); i++)
	{
		generateTangentDataThreads[i] = std::jthread(generateTangentData, &m_rawMeshes[i]);
	}
}

