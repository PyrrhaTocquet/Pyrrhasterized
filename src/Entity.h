#pragma once
#include "Drawable.h"
#include "Defs.h"
#include "Model.h"


class Entity {

protected:
	Model* m_model = nullptr;
	VulkanContext* m_context = nullptr;
public:
	//Model not initialized here ! Left to be initialized by the child classes
	Entity(VulkanContext* context);
	~Entity();
	virtual void update() = 0;
	Model* getModelPtr();

};