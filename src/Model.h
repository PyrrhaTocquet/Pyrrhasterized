#pragma once
#include "Defs.h"
#include <filesystem>
#include "VulkanImage.h"
#include "VulkanContext.h"

struct TexturedMesh {
	std::vector<Vertex> loadingVertices;
	std::vector<uint32_t> loadingIndices;
	uint32_t verticesCount = 0;
	uint32_t indicesCount = 0;
	VulkanImage* textureImage = nullptr;
	VulkanImage* normalMapImage = nullptr;
	uint32_t textureId;
	uint32_t normalMapId;
};

class Model {
private:
	VulkanContext* m_context = nullptr;
	std::vector<TexturedMesh> m_texturedMeshes;
	Transform m_transform;

	void loadModel(const std::filesystem::path& path);
	void loadGltf(const std::filesystem::path& path);
	void loadObj(const std::filesystem::path& path);
	void generateTangents();
public:
	Model(VulkanContext* context, const std::filesystem::path& path, const Transform& transform);
	Model();
	~Model();
	glm::mat4 getMatrix();
	void drawModel(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t& indexOffset, ModelPushConstant& pushConstant);
	std::vector<TexturedMesh>& getMeshes();

	void translateBy(glm::vec3 translation);
	void rotateBy(glm::vec3 rotation);
	void scaleBy(glm::vec3 scale);

	void clearLoadingVertexData();
	void clearLoadingIndexData();

};