#pragma once
#include "Defs.h"
#include <filesystem>
#include "VulkanImage.h"
#include "VulkanContext.h"
#include "Material.h"
#include "SerializationTools.h"
#include "GeometryTools.h"

struct RawMesh
{
	std::vector<Vertex> loadingVertices{};
	std::vector<uint32_t> loadingIndices{};
	uint32_t verticesCount{};
	uint32_t indicesCount{};
	uint32_t materialId{};
	Material *material = nullptr;
};


class Model {
private:
	VulkanContext* m_context = nullptr;
	std::filesystem::path m_path{};
	std::vector<RawMesh> m_rawMeshes{};
	std::vector<Mesh> m_meshes{};
	bool m_isLoaded = false;

	PFN_vkCmdDrawMeshTasksEXT vkDrawMeshTasks{};


	void loadGltf(const std::filesystem::path& path, bool isBaked);
	void generateTangents();
public:
	Model(VulkanContext* context, std::filesystem::path path);
	~Model();
	void loadModel();
	void drawModel(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant);
	[[nodiscard]]std::vector<Mesh>& getMeshes();
	[[nodiscard]]std::vector<RawMesh>& getRawMeshes();

	void clearLoadingVertexData();
	void clearLoadingIndexData();

	bool isLoaded(){ return m_isLoaded;};
	const std::filesystem::path& path(){ return m_path;};

};
