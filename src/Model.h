#pragma once
#include "Defs.h"
#include <filesystem>
#include "VulkanImage.h"
#include "VulkanContext.h"
#include "Material.h"

struct Mesh {
	std::vector<Vertex> loadingVertices;
	std::vector<uint32_t> loadingIndices;
	uint32_t verticesCount = 0;
	uint32_t indicesCount = 0;
	uint32_t materialId;
	Material* material;
};

class Model {
private:
	VulkanContext* m_context = nullptr;
	std::vector<Mesh> m_meshes;
	Transform m_transform;

	void loadModel(const std::filesystem::path& path);
	void loadGltf(const std::filesystem::path& path);
	void generateTangents();
public:
	Model(VulkanContext* context, const std::filesystem::path& path, const Transform& transform);
	Model();
	~Model();
	[[nodiscard]]glm::mat4 getMatrix();
	void drawModel(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t& indexOffset, ModelPushConstant& pushConstant);
	[[nodiscard]]std::vector<Mesh>& getMeshes();

	void translateBy(glm::vec3 translation);
	void rotateBy(glm::vec3 rotation);
	void scaleBy(glm::vec3 scale);

	void clearLoadingVertexData();
	void clearLoadingIndexData();

};