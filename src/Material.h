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
	glm::vec4 baseColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
	glm::vec4 emissiveColor = glm::vec4(0.f, 0.f, 0.f, 0.f);
	float metallicFactor = 0.f;
	float roughnessFactor = 0.5f;
	glm::uint alphaMode = static_cast<glm::uint>(AlphaMode::MaskAlphaMode);
	glm::float32 alphaCutoff = 0.5f;
	glm::uint hasAlbedoTexture = false;
	glm::uint hasNormalTexture = false;
	glm::uint hasMetallicRoughnessTexture = false;
	glm::uint hasEmissiveTexture = false;
	glm::uint albedoTextureId = 0;
	glm::uint normalTextureId = 0;
	glm::uint metallicRoughnessTextureId = 0;
	glm::uint emissiveTextureId = 0;
};

class Material {
public:
	uint32_t m_albedoTextureId = 0;
	uint32_t m_normalTextureId = 0;
	uint32_t m_metallicRoughnessTextureId = 0;
	uint32_t m_emissiveTextureId = 0;
private:
	VulkanContext* m_context = VK_NULL_HANDLE;

	glm::vec4 m_baseColorFactor = { 1.f, .766f, 0.336f, 1.f };
	glm::vec3 m_emissiveFactor = { 0.f, 0.f, 0.f };
	float m_metallicFactor = 0.f;
	float m_roughnessFactor = 1.f;
	AlphaMode m_alphaMode = AlphaMode::OpaqueAlphaMode;
	float m_alphaCutoff = 0.5f;

	VulkanImage* m_albedoTexture = nullptr;
	VulkanImage* m_metallicRoughnessTexture = nullptr; //Metalness : B, Roughness : G
	VulkanImage* m_normalTexture = nullptr;
	//VulkanImage* occlusionTexture; unsupported
	VulkanImage* m_emissiveTexture = nullptr;
	bool m_hasAlbedoTexture = false;
	bool m_hasNormalTexture = false;
	bool m_hasMetallicRoughnessTexture = false;
	bool m_hasEmissiveTexture = false;



public: static vk::Sampler s_baseColorSampler, s_metallicRoughnessSampler, s_normalSampler, s_emissiveSampler;

public:
	Material(VulkanContext* context);
	~Material();
	Material* setBaseColor(const glm::vec4& color);
	Material* setEmissiveFactor(const glm::vec3& factor);
	Material* setMetallicFactor(float factor);
	Material* setRoughnessFactor(float factor);
	Material* setAlphaMode(AlphaMode alphaMode);
	Material* setAlbedoTexture(VulkanImage* texture);
	Material* setMetallicRoughnessTexture(VulkanImage* texture);
	Material* setNormalTexture(VulkanImage* texture);
	Material* setEmissiveTexture(VulkanImage* texture);
	Material* setAlphaCutoff(float cutoff);

	MaterialUBO getUBO();
	VulkanImage* getAlbedoTexture();
	VulkanImage* getNormalTexture();
	VulkanImage* getEmissiveTexture();
	VulkanImage* getMetallicRoughnessTexture();

	bool hasAlbedoTexture();
	bool hasNormalTexture();
	bool hasMetallicRoughness();
	bool hasEmissiveTexture();

	static void createSamplers(VulkanContext* context);
	static void cleanSamplers(VulkanContext* context);
	
};