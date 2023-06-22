#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(VulkanContext* context, const glm::vec4& direction, const glm::vec4& lightColor) : Light(context, lightColor)
{
	m_lightType = LightType::DirectionalLightType;
	m_directionWorld = glm::normalize(direction);
}

DirectionalLight::DirectionalLight(VulkanContext* context, const glm::vec4& direction) : DirectionalLight(context, direction, m_lightColor)
{
	
}


//Make sure the camera reference is set !
void DirectionalLight::update()
{
	m_directionView = glm::normalize(m_camera->getViewMatrix() * m_directionWorld);
}

//Populates the uniform data struct
LightUBO DirectionalLight::getUniformData()
{
	return LightUBO{
		.directionWorld = m_directionWorld,
		.directionView = m_directionView,
		.lightColor = m_lightColor,
		.intensity = m_intensity,
		.enabled = m_enabled,
		.type = static_cast<uint32_t>(m_lightType),
	};
}

glm::vec4 DirectionalLight::getWorldDirection()
{
	return m_directionWorld;
}


