/*
author: Pyrrha Tocquet
date: 20/06/23
desc: Manages directional light data
*/
#pragma once
#include "Light.h"


class DirectionalLight : public Light {
	glm::vec4 m_directionWorld;
	glm::vec4 m_directionView;

	//TODO Proper sun and not this
	glm::vec4 m_firstDirectionWorld;

public:
	DirectionalLight(VulkanContext* context, const glm::vec4& direction);
	DirectionalLight(VulkanContext* context, const glm::vec4& direction, const glm::vec4& lightColor);
	virtual void update()override;
	virtual LightUBO getUniformData()override;
	glm::vec4 getWorldDirection();
	void setShadowCaster();
};