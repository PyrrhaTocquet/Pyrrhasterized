#pragma once
#include "Drawable.h"
#include "Defs.h"
#include "Model.h"


class Entity : public Drawable {

private:
	Model m_model;
public:
	Entity();
	virtual void update();
};