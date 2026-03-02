#include "Spotlight.h"

Spotlight::Spotlight(VulkanContext* context, const glm::vec4& position, const glm::vec4& direction, float spotlightAngle, float range) : Spotlight(context, position, direction, spotlightAngle, range, m_lightColor)
{
}

Spotlight::Spotlight(VulkanContext* context, const glm::vec4& position, const glm::vec4& direction, float spotlightAngle, float range, const glm::vec4& lightColor) 
	: Light(context, lightColor)
	, m_positionWorld{position}
	, m_positionView{}
	, m_directionWorld{glm::normalize(direction)}
	, m_directionView{}
	, m_spotlightAngle{spotlightAngle}
	, m_range{range}
{
	m_lightType = LightType::SpotlightType;
}

//Make sure the camera reference is set !
void Spotlight::update(VulkanScene*)
{
	glm::mat4 viewMat = m_camera->getViewMatrix();
	m_positionView = viewMat * m_positionWorld;
	m_directionView = glm::normalize(viewMat * m_directionWorld);
}

LightUBO Spotlight::getUniformData()
{
	return LightUBO{
		.positionWorld = m_positionWorld,
		.directionWorld = m_directionWorld,
		.positionView = m_positionView,
		.directionView = m_directionView,
		.lightColor = m_lightColor,
		.spotlightHalfAngle = m_spotlightAngle,
		.range = m_range,
		.intensity = m_intensity,
		.enabled = m_enabled,
		.type = static_cast<uint32_t>(m_lightType),
	};
}
