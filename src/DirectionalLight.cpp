#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(VulkanContext* context, const glm::vec4& direction, const glm::vec4& lightColor) : Light(context, lightColor)
{
	m_lightType = LightType::DirectionalLightType;
	m_firstDirectionWorld = direction;
}

DirectionalLight::DirectionalLight(VulkanContext* context, const glm::vec4& direction) : DirectionalLight(context, direction, m_lightColor)
{
	
}


//Make sure the camera reference is set !
void DirectionalLight::update()
{
	//TODO CLEAN THIS STUFF
	if (m_shadowCaster)
	{
		float time = m_context->getTime().elapsedSinceStart;
		float offset = 15 * sin(time/4);
		m_directionWorld = glm::normalize(glm::vec4(m_firstDirectionWorld.x, m_firstDirectionWorld.y, m_firstDirectionWorld.z + offset, m_firstDirectionWorld.w));
	}
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
		.shadowCaster = m_shadowCaster,
	};
}

glm::vec4 DirectionalLight::getWorldDirection()
{
	return m_directionWorld;
}

void DirectionalLight::setShadowCaster()
{
	m_shadowCaster = true;
}


