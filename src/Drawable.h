/*
author: Pyrrha Tocquet
date: 30/05/23
desc: Abstract class for drawable objects. Mainly defines member functions that manage command recording.
*/

#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "Defs.h"


class Drawable {
public :
	Drawable();
	virtual void draw(vk::CommandBuffer commandBuffer, uint32_t currentFrame, vk::PipelineLayout pipelineLayout, ModelPushConstant& pushConstant) = 0;
};