#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"
#include <GLFW/glfw3.h>
#include "VulkanContext.h"
#include "VulkanTools.h"
#include "glm/glm.hpp"

class VulkanContext;
struct VulkanImageParams {
	uint32_t width;
	uint32_t height;
	uint32_t mipLevels;
	vk::SampleCountFlagBits numSamples;
	vk::Format format;
	vk::ImageTiling tiling;
	vk::ImageUsageFlags usage;
	bool useDedicatedMemory = false; //Set to true for large images that can be destroyed and recreated with different sizes (framebuffer attachments)
	
};

struct VulkanImageViewParams {
	vk::ImageAspectFlags aspectFlags;
};

class VulkanImage
{
public:
	vk::Image m_image = VK_NULL_HANDLE;
	vk::ImageView m_imageView = VK_NULL_HANDLE;
private:
	void constructVkImage(VulkanContext* context, VulkanImageParams imageParams);
	void constructVkImageView(VulkanContext* context, VulkanImageParams imageParams, VulkanImageViewParams imageViewParams);
	vk::Device m_device;
	bool m_loadingFailed = false;

private:
	vma::Allocator m_allocator;
	vma::Allocation m_allocation;
public:
	//General single Image constructor
	VulkanImage(VulkanContext* context, VulkanImageParams imageParams, VulkanImageViewParams imageViewParams);

	//General texture Image constructor
	VulkanImage(VulkanContext* context, VulkanImageParams imageParams, VulkanImageViewParams imageViewParams, std::string path);
	~VulkanImage();

	void transitionImageLayout(VulkanContext* context, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
	void generateMipmaps(VulkanContext* context, vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	bool hasLoadingFailed();
};

