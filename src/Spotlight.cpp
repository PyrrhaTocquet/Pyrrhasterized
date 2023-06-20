#include "Spotlight.h"

Spotlight::Spotlight(VulkanContext* context, const glm::vec4& position, const glm::vec4& direction, float spotlightAngle) : Spotlight(context, position, direction, spotlightAngle, m_lightColor)
{

}

Spotlight::Spotlight(VulkanContext* context, const glm::vec4& position, const glm::vec4& direction, float spotlightAngle, const glm::vec4& lightColor) : Light(context, lightColor)
{
	m_lightType = LightType::SpotlightType;
	m_positionWorld = position;
	m_directionWorld = direction;
	m_spotlightAngle = spotlightAngle;
}

//Make sure the camera reference is set !
void Spotlight::update()
{
	glm::mat4 viewMat = m_camera->getViewMatrix();
	m_positionView = viewMat * m_positionWorld;
	m_directionView = viewMat * m_directionWorld;
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
		.intensity = m_intensity,
		.enabled = m_enabled,
		.type = static_cast<uint32_t>(m_lightType),
	};
}
