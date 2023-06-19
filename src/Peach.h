/*
author: Pyrrha Tocquet
date: 01/06/23
desc: Demo entity that shows that you can and how you can script entities
Peach is controllable horizontally with OKLM keys
*/


#pragma once
#include "Defs.h"
#include "Entity.h"

class Peach : public Entity {
private:

public:
	Peach(VulkanContext* context);
	void update() override;
};