/*
author: Pyrrha Tocquet
date: 01/06/23
desc: Entity managing camera related data and camera movement
*/

#pragma once
#include "Defs.h"
#include "Entity.h"


struct CameraCoords {
	glm::vec3 pitchYawRoll = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 cameraPos = glm::vec3(2.0, 1.0f, 0.0f);

	[[nodiscard]] glm::vec3 getDirection() {
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
	const float cameraSpeed = 3.f;
	const float fastCameraSpeed = 20.f;

	Camera(VulkanContext* context);
	void update() override;

	[[nodiscard]] glm::mat4 getViewMatrix();
	[[nodiscard]] glm::mat4 getProjMatrix(VulkanContext* context);
	[[nodiscard]] glm::vec3 getCameraPos();
};