#pragma once
#include "Defs.h"
#include "Entity.h"


struct CameraCoords {
	glm::vec3 pitchYawRoll = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 cameraPos = glm::vec3(2.0, 1.0f, 0.0f);

	glm::vec3 getDirection() {
		glm::vec3 direction;
		direction.x = cos(glm::radians(pitchYawRoll.x)) * cos(glm::radians(pitchYawRoll.y));
		direction.y = -sin(glm::radians(pitchYawRoll.x));
		direction.z = cos(glm::radians(pitchYawRoll.x)) * sin(glm::radians(pitchYawRoll.y));
		direction = glm::normalize(direction);
		return direction;
	}

};


class Camera : public Entity {
private:
	CameraCoords m_cameraCoords;

public:
	const float nearPlane = 0.1f;
	const float farPlane = 150.f;

	Camera(VulkanContext* context);
	void update() override;

	glm::mat4 getViewMatrix();
	glm::mat4 getProjMatrix(VulkanContext* context);
	glm::vec3 getCameraPos();
};