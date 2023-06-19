#include "Entity.h"


Entity::Entity(VulkanContext* context)
{
	m_context = context;
}

Entity::~Entity()
{
	delete m_model;
}

//Returns a pointer of the entity's model
Model* Entity::getModelPtr()
{
	if (m_model == nullptr) {
		std::runtime_error("Tried to retrieve a model that has no been initialized");
	}
	return m_model;
}
