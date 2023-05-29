#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>

//Abstract class for drawable objects. Mainly defines member functions that manage command recording
class Drawable {
public :
	Drawable();
	virtual ~Drawable();
	virtual void draw();
};