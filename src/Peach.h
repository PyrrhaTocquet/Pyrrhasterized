#pragma once
#include "Defs.h"
#include "Entity.h"

class Peach : public Entity {
private:

public:
	Peach(VulkanContext* context);
	void update() override;
};