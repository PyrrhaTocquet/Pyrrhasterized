/*
author: Pyrrha Tocquet
date: 01/06/23
desc: Demo entity that shows that you can and how you can script entities
Peach is controllable horizontally with OKLM keys
*/


#pragma once
#include "Defs.h"
#include "Entity.h"
#include "VulkanScene.h"

class Peach : public Entity {
private:
	
public:
	uint32_t m_node;
	Peach(VulkanContext* context, VulkanScene *scene, int parent = -1);
	void update(VulkanScene *scene) override;
};