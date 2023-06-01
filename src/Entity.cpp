#include "Entity.h"


Entity::Entity(VulkanContext* context)
{
	m_context = context;
}

Entity::~Entity()
{
	delete m_model;
}

Model* Entity::getModelPtr()
{
	if (m_model == nullptr) {
		std::runtime_error("Tried to retrieve a model that has no been initialized");
	}
	return m_model;
}
