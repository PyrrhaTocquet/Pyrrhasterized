#include "PointLight.h"

PointLight::PointLight(VulkanContext* context, const glm::vec4& position, float range) : PointLight(context, position, range, glm::vec4(1.f, 1.f, 1.f, 1.f))
{

}

PointLight::PointLight(VulkanContext* context, const glm::vec4& position, float range, const glm::vec4 lightColor) : Light(context, lightColor)
{
	m_lightType = LightType::PointLightType;
	m_positionWorld = position;
	m_range = range;
}

//Make sure the camera reference is set !
void PointLight::update()
{
	m_positionView = m_camera->getViewMatrix() * m_positionWorld;
}

LightUBO PointLight::getUniformData()
{
	return LightUBO{
	.positionWorld = m_positionWorld,
	.positionView = m_positionView,
	.lightColor = m_lightColor,
	.range = m_range,
	.intensity = m_intensity,
	.enabled = m_enabled,
	.type = static_cast<uint32_t>(m_lightType),
	};
}
