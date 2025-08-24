/*
author: Pyrrha Tocquet
date: 01/06/23
desc: Abstract class for Models that have behavior. The update function is called every frame before rendering
note: The model is not initialized by the entity and is left to be initialized by child classees
*/

#pragma once
#include "Defs.h"
#include "VulkanContext.h"
//#include "VulkanScene.h"

class VulkanScene;

class Entity {

protected:
	VulkanContext* m_context = nullptr;
public:
	Entity(VulkanContext* context);
	~Entity();
	virtual void update(VulkanScene *scene) = 0;
};