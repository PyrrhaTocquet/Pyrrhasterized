#include "VulkanTools.h"

namespace vkTools {
	vk::Sampler createSampler(const vk::SamplerCreateInfo& samplerInfo, const vk::Device& device)
	{
		vk::Sampler sampler;
        try {
            sampler = device.createSampler(samplerInfo);
        }
        catch (vk::SystemError err)
        {
            throw std::runtime_error("failed to create texture sampler");
        }
        return sampler;
	}
}