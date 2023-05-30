#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include "glm/glm.hpp"

/* RENDERING CONSTS*/
const bool ENABLE_MSAA = false;

/* ENUMS */
enum RenderPassesId {
	ShadowMappingPassId = 0,
	MainRenderPassId,
};

/* STRUCTS */
struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 lightView;
	glm::mat4 lightProj;
};

struct ModelPushConstant {
	glm::mat4 model;
	glm::int32 textureId;
	glm::int32 normalMapId;
	glm::float32 time;
	glm::vec2 data;
};

struct PipelineInfo {
	const char* vertPath;
	const char* fragPath;
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlags cullmode = vk::CullModeFlagBits::eBack;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	float lineWidth = 1.0f;
	vk::Bool32 depthTestEnable = VK_TRUE;
	vk::Bool32 depthWriteEnable = VK_TRUE;
	RenderPassesId renderPassId = RenderPassesId::MainRenderPassId;
	bool isMultisampled = true;
};

struct VulkanPipeline {
	PipelineInfo pipelineInfo;
	vk::Pipeline pipeline;
};

struct CameraCoords {
	glm::vec3 pitchYawRoll = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 cameraPos = glm::vec3(2.0, 1.0f, 0.0f);

	glm::vec3 getDirection() {
		glm::vec3 direction;
		direction.x = cos(glm::radians(pitchYawRoll.x)) * cos(glm::radians(pitchYawRoll.y));
		direction.y = -sin(glm::radians(pitchYawRoll.x));
		direction.z = cos(glm::radians(pitchYawRoll.x)) * sin(glm::radians(pitchYawRoll.y));
		direction = glm::normalize(direction);
		return direction;
	}

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