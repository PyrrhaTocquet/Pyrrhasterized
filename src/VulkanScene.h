#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "Defs.h"
#include <filesystem>
#include "Model.h"
#include "Drawable.h"
#include "DirectionalLight.h"
#include <future>
#include <thread>
#include <set>

#define MAX_SCENE_DEPTH 16

class VulkanScene : Drawable
{
public :

	struct Hierarchy
	{
		int	parent = -1;
		int	firstChild = -1;
		int	nextSibling = -1;
		int	lastSibling = -1;
		int	level = 0;
	};

	struct DirtyableTransform
	{
		glm::mat4	transform;
		bool		isDirty;
	};

	VulkanBuffer m_meshletInfoBuffer, m_primitiveBuffer, m_indexBuffer, m_vertexBuffer;

	// Scene Tree
	std::vector<Transform> m_localTransforms;
	std::vector<glm::mat4> m_globalTransforms;
	std::vector<Hierarchy> m_hierarchies;
	std::vector<Model> m_models; 
	std::vector<std::string> m_nodeNames;

	std::unordered_map<uint32_t, uint32_t> modelForNode;
	//std::unordered_map<uint32_t, uint32_t> materialForNode; //TODO

	std::vector<uint32_t> m_changedThisFrame[MAX_SCENE_DEPTH];


	std::vector<uint32_t> m_modelsToLoad;
	std::vector<Light*> m_lights;


	// Uniform Buffers
	std::vector<VulkanBuffer> m_generalUniformBuffers;
	std::vector<VulkanBuffer> m_lightUniformBuffers;
	std::vector<VulkanBuffer> m_shadowCascadeUniformBuffers;
	std::array<std::vector<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_materialUniformBuffers;

	uint32_t m_materialCount = 0;
private:
	VulkanContext* m_context;

	uint32_t m_meshletCount = 0, m_primitiveCount = 0, m_indexCount = 0, m_vertexCount = 0;
	vma::Allocator* m_allocator;
	DirectionalLight* m_sun;
	std::array<CascadeUniformObject, MAX_FRAMES_IN_FLIGHT> m_cascadeUbos;

	Camera* m_camera;

	vk::DescriptorPool m_geometryDescriptorPool;
	vk::DescriptorSet m_geometryDescriptorSet;
public:
	VulkanScene(VulkanContext* context, DirectionalLight* sun);
	~VulkanScene();
	uint32_t addMeshNode(const std::filesystem::path& path, Transform transform, int parent = -1);
	void updateTransforms();
	void dirtyNode(uint32_t node);
	uint32_t addMeshNode(Model &model, Transform transform, int parent = -1);
	uint32_t addNode(Transform transform, std::string name = "empty Node", int parent = -1);
	void createGeometryDescriptorSet(vk::DescriptorSetLayout geometryDescriptorSetLayout);
	[[nodiscard]]	vk::DescriptorSet getGeometryDescriptorSet();
	void loadModels();
	void createGeometryBuffers();
	[[nodiscard]]	const uint32_t getIndexBufferSize();
	void addLight(Light* light);
	[[nodiscard]]	std::vector<Light*> getLights();
	void updateLights();
	[[nodiscard]]	DirectionalLight* getSun();
	void draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant) override;
	void	createUniformBuffers();
	void	updateUniformBuffers(uint32_t m_currentFrame);
	void	setCamera(Camera *camera);

	void	translateNode(uint32_t node, glm::vec3 translation);
	void	rotateNode(uint32_t node, glm::vec3 rotation);
	void	scaleNode(uint32_t node, glm::vec3 scale);

	[[nodiscard]]	const VulkanBuffer getGeneralUniformBuffer(uint32_t currentFrame) { return m_generalUniformBuffers[currentFrame]; };
	[[nodiscard]] const VulkanBuffer getLightUniformBuffer(uint32_t currentFrame) {
		return m_lightUniformBuffers[currentFrame];
	};
	[[nodiscard]] const VulkanBuffer getShadowCascadeUniformBuffer(uint32_t currentFrame) {
		return m_shadowCascadeUniformBuffers[currentFrame];
	};
	[[nodiscard]] const VulkanBuffer getMaterialUniformBuffer(uint32_t currentFrame, uint32_t materialId) {
		return m_materialUniformBuffers[currentFrame][materialId]; // Not sure it's the material id here
	};
	[[nodiscard]]	std::vector<vk::DescriptorImageInfo> generateTextureImageInfo();
private:
	void createVertexBuffer();
	void createIndexBuffer();
	void updateGeneralUniformBuffer(uint32_t currentFrame);
	void updateLightUniformBuffer(uint32_t currentFrame);
	void updateShadowCascadeUniformBuffer(uint32_t currentFrame);
};

