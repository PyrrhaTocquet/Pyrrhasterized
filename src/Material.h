/*
author: Pyrrha Tocquet
date: 27/06/23
desc: Manages material data, and ubo struct creation
Constructed with a builder-like pattern
*/

#pragma once
#include "Defs.h"
#include "VulkanImage.h"
#include "VulkanContext.h"
#include "VulkanTools.h"

struct MaterialUBO {
	glm::vec4 baseColor;
	glm::vec4 emissiveColor;
	float metallicFactor;
	float roughnessFactor;
	float padding[2] = { 0.f, 0.f };
};

class Material {
private:
	VulkanContext* m_context = VK_NULL_HANDLE;

	glm::vec4 m_baseColorFactor = { 1.f, 1.f, 1.f, 1.f };
	glm::vec3 m_emissiveFactor = { 0.f, 0.f, 0.f };
	float m_metallicFactor = 1.f;
	float m_roughnessFactor = 1.f;
	AlphaMode m_alphaMode = AlphaMode::OpaqueAlphaMode;


	VulkanImage* m_baseColorTexture = nullptr;
	VulkanImage* m_metallicRoughnessTexture = nullptr; //Metalness : B, Roughness : G
	VulkanImage* m_normalTexture = nullptr;
	//VulkanImage* occlusionTexture; unsupported
	VulkanImage* m_emissiveTexture = nullptr;

public: static vk::Sampler s_baseColorSampler, s_metallicRoughnessSampler, s_normalSampler, s_emissiveSampler;

public:
	Material(VulkanContext* context);
	Material* setBaseColor(const glm::vec4& color);
	Material* setEmissiveFactor(const glm::vec3& factor);
	Material* setMetallicFactor(float factor);
	Material* setRoughnessFactor(float factor);
	Material* setAlphaMode(AlphaMode alphaMode);
	Material* setBaseColorTexture(VulkanImage* texture);
	Material* setMetallicRoughnessTexture(VulkanImage* texture);
	Material* setNormalTexture(VulkanImage* texture);
	Material* setEmissiveTexture(VulkanImage* texture);

	MaterialUBO getUBO();

	static void createSamplers(VulkanContext* context);
	static void cleanSamplers(VulkanContext* context);
	
};