/*
author: Pyrrha Tocquet
date: 20/06/23
desc: Manages SpotLight data
*/
#pragma once
#include "Light.h"

class Spotlight : public Light {
	glm::vec4 m_positionWorld;
	glm::vec4 m_positionView;
	glm::vec4 m_directionWorld;
	glm::vec4 m_directionView;
	float m_spotlightAngle;
	float m_range;
public :
	Spotlight(VulkanContext* context, const glm::vec4& position, const glm::vec4& direction, float spotlightAngle, float range);
	Spotlight(VulkanContext* context, const glm::vec4& position, const glm::vec4& direction, float spotlightAngle, float range, const glm::vec4& lightColor);
	virtual void update()override;
	virtual LightUBO getUniformData()override;
};