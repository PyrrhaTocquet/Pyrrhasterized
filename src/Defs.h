#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <filesystem>
#include <assert.h>

/* RENDERING CONSTS*/
const bool ENABLE_MSAA = false;
const uint32_t SHADOW_CASCADE_COUNT = 4;
const uint32_t MAX_LIGHT_COUNT = 10;
const uint32_t MAX_TEXTURE_COUNT = 4096;
const uint32_t MAX_MATERIAL_COUNT = 4096;

const std::filesystem::path BAKED_ASSETS_PATH = "baked_assets/";

/* ENUMS */
enum RenderPassesId {
	ShadowMappingPassId = 0,
	MainRenderPassId,
};

enum AlphaMode {
	OpaqueAlphaMode,
	MaskAlphaMode,
	TransparentAlphaMode
};

/* STRUCTS */
struct GeneralUniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 cascadeViewProj[4];
	float cascadeSplits[4] = {0.f};
	glm::vec3 cameraPos;
	float shadowMapsBlendWidth;
	float time;
	float hairLength;
	float gravityFactor;
	float hairDensity;
};

struct CascadeUniformObject {
	glm::mat4 cascadeViewProjMat[4];
	float cascadeSplits[4];
};

struct ModelPushConstant {
	glm::mat4 model;
	glm::int32 materialId;
	glm::uint32 cascadeId;
	uint32_t meshlet;
	uint32_t meshletCount;
	uint32_t shellCount = 8;
	//float padding[5];
};

struct Time {
	float elapsedSinceStart;
	float deltaTime;
};

struct MeshletIndexingInfo{
	glm::vec4 boundingSphere = glm::vec4();
	uint32_t vertexCount = 0;
	uint32_t vertexOffset = 0;
	uint32_t primitiveCount = 0;
	uint32_t primitiveOffset = 0;
	uint32_t meshletId = 0;
	uint64_t padding = 0;
};


struct Transform {
	glm::vec3 translate = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 rotate = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);

	bool hasChanged = true;
	glm::mat4 transformMatrix = glm::mat4();

	glm::mat4 computeMatrix() {

		if (hasChanged) {

			glm::mat4 translateMatrix = glm::translate(glm::mat4(1.f), translate);
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.f), glm::radians(rotate.x), glm::vec3(1.f, 0.f, 0.f));
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotate.y), glm::vec3(0.f, 1.f, 0.f));
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotate.z), glm::vec3(0.f, 0.f, 1.f));
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.f), scale);
			transformMatrix = translateMatrix * rotationMatrix * scaleMatrix;
			hasChanged = false;
		}

		return transformMatrix;
	};
};

struct Vertex {
	glm::vec3 pos;
	float padding;
	glm::vec2 texCoord;
	float padding2[2];
	glm::vec3 normal;
	float padding3;
	glm::vec4 tangent; //w: handles handedness


	static vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription bindingDescription;
		bindingDescription.binding = 0,
			bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;
		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = vk::Format::eR32G32B32A32Sfloat;
		attributeDescriptions[3].offset = offsetof(Vertex, tangent);


		return attributeDescriptions;
	}
};

struct Meshlet{
	struct Triangle{
		uint32_t i0;
		uint32_t i1;
		uint32_t i2;
	};
	std::vector<uint32_t> uniqueVertexIndices;
	std::vector<Triangle> primitiveIndices;
	MeshletIndexingInfo meshletInfo;
};

struct Mesh {
	std::vector<Meshlet> meshlets;
	std::vector<Vertex> vertices;
};

struct VulkanBuffer 
{
	vk::Buffer m_Buffer;
	vma::Allocation m_Allocation;
};


/* CLEAR COLORS */
constexpr vk::ClearColorValue CLEAR_COLOR = vk::ClearColorValue(std::array<float, 4>{.8f, .8f, .8f, 1.0f});
constexpr vk::ClearDepthStencilValue CLEAR_DEPTH = vk::ClearDepthStencilValue({ 1.0f, 0 });

constexpr std::array<vk::ClearValue, 2> setMainClearColors() {
	std::array<vk::ClearValue, 2> clearValues{};
	clearValues[1].setDepthStencil(CLEAR_DEPTH);
	clearValues[0].setColor(CLEAR_COLOR);
	return clearValues;
}

constexpr std::array<vk::ClearValue, 1> setShadowClearColors() {
	std::array<vk::ClearValue, 1> clearValues{};
	clearValues[0].setDepthStencil(CLEAR_DEPTH);
	return clearValues;
}
constexpr std::array<vk::ClearValue, 2> MAIN_CLEAR_VALUES = setMainClearColors();
constexpr std::array<vk::ClearValue, 1> SHADOW_DEPTH_CLEAR_VALUES = setShadowClearColors();