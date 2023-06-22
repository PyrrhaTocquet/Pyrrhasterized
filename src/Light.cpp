#include "Light.h"

Light::Light(VulkanContext* context, const glm::vec4& lightColor) : Entity(context)
{
	m_lightColor = lightColor;
}

Light::Light(VulkanContext* context) : Entity(context)
{
}

void Light::setIntensity(float intensity)
{
	m_intensity = intensity;
}

//Sets the camera reference for later view matrix operations
void Light::setCamera(Camera* camera)
{
	m_camera = camera;
}
