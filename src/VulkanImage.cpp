#include "VulkanImage.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//creates the VkImage and Allocation part of the VulkanImage
void VulkanImage::constructVkImage(VulkanContext* context, VulkanImageParams imageParams)
{
	vk::ImageCreateInfo imageInfo{
		.imageType = vk::ImageType::e2D,
		.format = imageParams.format,
		.extent {
			.width = static_cast<uint32_t>(imageParams.width),
			.height = static_cast<uint32_t>(imageParams.height),
			.depth = 1,
		},
		.mipLevels = imageParams.mipLevels,
		.arrayLayers = 1,	
		.samples = imageParams.numSamples,
		.tiling = imageParams.tiling,
		.usage = imageParams.usage,
		.sharingMode = vk::SharingMode::eExclusive,
		.initialLayout = vk::ImageLayout::eUndefined,
	};
	
	vma::AllocationCreateInfo allocInfo{
		.usage = vma::MemoryUsage::eAuto,
	};
	if (imageParams.useDedicatedMemory)
	{
		allocInfo.flags = vma::AllocationCreateFlagBits::eDedicatedMemory;
	}

	std::tie(m_image, m_allocation) = m_allocator.createImage(imageInfo, allocInfo); //TODO error checking ?


}

//builds the image view part of the VulkanImage
void VulkanImage::constructVkImageView(VulkanContext* context, VulkanImageParams imageParams, VulkanImageViewParams imageViewParams)
{
	vk::ImageViewCreateInfo imageViewInfo{
		.image = m_image,
		.viewType = vk::ImageViewType::e2D,
		.format = imageParams.format,
		.subresourceRange {
			.aspectMask = imageViewParams.aspectFlags,
			.baseMipLevel = 0,
			.levelCount = imageParams.mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
	};
	
	try {
		m_imageView = context->getDevice().createImageView(imageViewInfo);
	}
	catch (vk::SystemError err)
	{
		throw std::runtime_error("could not create image view");
	}
	
}

//Constructor for general purpose images
VulkanImage::VulkanImage(VulkanContext* context, VulkanImageParams imageParams, VulkanImageViewParams imageViewParams)
{
	m_allocator = context->getAllocator();
	m_device = context->getDevice();
	constructVkImage(context, imageParams);
	constructVkImageView(context, imageParams, imageViewParams);
}

//Constructor for textures
VulkanImage::VulkanImage(VulkanContext* context, VulkanImageParams imageParams, VulkanImageViewParams imageViewParams, std::string path)
{
	m_allocator = context->getAllocator();
	m_device = context->getDevice();

	//Texture file read
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); //Forces the image to be loaded with an alpha channel for consistency
	if (!pixels) {
		std::cerr << "Image file " << path << " was not loaded" << std::endl;
		m_loadingFailed = true;
		return;
	}

	//Highest number possible of miplevels
	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	//Staging buffer (CPU visible)
	vk::Buffer stagingBuffer;
	vma::Allocation stagingBufferAllocation;
	vk::DeviceSize imageSize = texWidth * texHeight * 4;
	std::tie(stagingBuffer, stagingBufferAllocation) = context->createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuToGpu);

	//Copy the pixel values to the buffer
	void* data = context->getAllocator().mapMemory(stagingBufferAllocation);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	context->getAllocator().unmapMemory(stagingBufferAllocation);

	//Remember to free !
	stbi_image_free(pixels);

	//VkImage
	imageParams.height = texHeight;
	imageParams.width = texWidth;
	imageParams.mipLevels = mipLevels;
	imageParams.usage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
	constructVkImage(context, imageParams);

	//Copy the staging buffer to the texture image
	transitionImageLayout( context, imageParams.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, imageParams.mipLevels); 
	context->copyBufferToImage(stagingBuffer, m_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	m_allocator.destroyBuffer(stagingBuffer, stagingBufferAllocation);


	//Generating the mip maps transitions the image to shader reading layout
	generateMipmaps(context, m_image, imageParams.format, imageParams.width, imageParams.height, imageParams.mipLevels);

	//Image View
	constructVkImageView(context, imageParams, imageViewParams);
}


VulkanImage::~VulkanImage()
{
	m_allocator.destroyImage(m_image, m_allocation);
	m_device.destroyImageView(m_imageView);

}


//Generates mipmaps for an image and converts it to shader read layout
void VulkanImage::generateMipmaps(VulkanContext* context, vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
	vk::FormatProperties formatProperties = context->getFormatProperties(imageFormat);

	//Required for blitting
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
	{
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	vk::CommandBuffer commandBuffer = context->beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier{
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = m_image,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	//recording each VkCMdBlitImage commands
	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, barrier);

		vk::ImageBlit blit{
			.srcSubresource = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.mipLevel = i - 1, //Source mip level
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			
			.dstSubresource = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.mipLevel = i, //dest mip level
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 }; //Determines the 3D region that the data will be blitted from
		blit.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
		blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
		blit.dstOffsets[1] = vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };//divide by two !

		commandBuffer.blitImage(m_image, vk::ImageLayout::eTransferSrcOptimal, m_image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);
		
		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;

	}
	
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);

	context->endSingleTimeCommands(commandBuffer);
}

bool VulkanImage::hasLoadingFailed()
{
	return m_loadingFailed;
}


//Transition the image from the oldLayout to a newLayout
void VulkanImage::transitionImageLayout(VulkanContext* context, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels) 
{
	vk::CommandBuffer commandBuffer = context->beginSingleTimeCommands();


	vk::ImageMemoryBarrier barrier{
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, //Only useful to transfer queue family ownerships
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = m_image,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	// Barrier access Masks
	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	// From Buffer to GPU Texture (GPU Texture layout) | From GPU Texture to shader reading layout
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite; // Pre-barrier operations (top of pipeline)

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe; // Pre-barrier operations (top of pipeline)
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);

	context->endSingleTimeCommands(commandBuffer);
}
