/*
author: Pyrrha Tocquet
date: 20/06/23
desc: Manages Point Light data
*/
#pragma once
#include "Light.h"

class PointLight : public Light {
	glm::vec4 m_positionWorld;
	glm::vec4 m_positionView = glm::vec4();
	float m_range;
public:
	PointLight(VulkanContext* context, const glm::vec4& position, float range);
	PointLight(VulkanContext* context, const glm::vec4& position, float range, const glm::vec4 lightColor);
	virtual void update()override;
	virtual LightUBO getUniformData()override;
};