/*
author: Pyrrha Tocquet
date: 20/06/23
desc: Abstract class for light types.
*/
#pragma once
#include "Defs.h"
#include "Entity.h"
#include "VulkanContext.h"
#include "Camera.h"

struct LightUBO {
	glm::vec4 positionWorld;
	glm::vec4 directionWorld;
	glm::vec4 positionView;
	glm::vec4 directionView;
	glm::vec4 lightColor;
	float spotlightHalfAngle;
	float range;
	float intensity;
	glm::uint32 enabled;
	glm::uint32 type;
	glm::uint32 shadowCaster;
	glm::vec2 padding;
};

enum LightType {
	DirectionalLightType = 0,
	PointLightType = 1,
	SpotlightType = 2
};

class Light : public Entity {
protected:
	Camera* m_camera;
	glm::vec4 m_lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	float m_intensity = 1.0f;
	LightType m_lightType;
	bool m_enabled = true;
	bool m_shadowCaster = false;
public:
	Light(VulkanContext* context, const glm::vec4& lightColor);
	Light(VulkanContext* context);
	void setCamera(Camera* camera);
	virtual void update()override = 0;
	virtual LightUBO getUniformData() = 0;
	void setIntensity(float intensity);
};