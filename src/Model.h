#include "Defs.h"
#include <filesystem>
#include "VulkanImage.h"
#include "VulkanContext.h"

struct TexturedMesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
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
	void drawModel(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, float time, uint32_t& indexOffset);
	std::vector<TexturedMesh>& getMeshes();

};