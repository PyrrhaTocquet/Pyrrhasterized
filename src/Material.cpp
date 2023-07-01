#include "Material.h"

vk::Sampler Material::s_baseColorSampler = VK_NULL_HANDLE;
vk::Sampler Material::s_metallicRoughnessSampler = VK_NULL_HANDLE;
vk::Sampler Material::s_normalSampler = VK_NULL_HANDLE;
vk::Sampler Material::s_emissiveSampler = VK_NULL_HANDLE;

Material::Material(VulkanContext* context) {
    m_context = context;
}

void Material::cleanSamplers(VulkanContext* context) {
    vk::Device device = context->getDevice();

    device.destroySampler(s_baseColorSampler);
    device.destroySampler(s_normalSampler);
    device.destroySampler(s_emissiveSampler);
    device.destroySampler(s_metallicRoughnessSampler);
}


void Material::createSamplers(VulkanContext* context)
{
	//Base Color Sampler
	vk::PhysicalDeviceProperties properties = context->getProperties();
    vk::Device device = context->getDevice();

    vk::SamplerCreateInfo samplerInfo{
      .magFilter = vk::Filter::eLinear, //Linear filtering
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eRepeat,
      .addressModeV = vk::SamplerAddressMode::eRepeat,
      .addressModeW = vk::SamplerAddressMode::eRepeat,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_TRUE, //Anisotropic Filtering
      .maxAnisotropy = properties.limits.maxSamplerAnisotropy, //Max texels samples to calculate the final color
      .compareEnable = VK_FALSE, //If enabled, texels will be compared to a value and the result of that compararison is used in filtering operations
      .compareOp = vk::CompareOp::eAlways,
      .minLod = 0.0f,
      .maxLod = VK_LOD_CLAMP_NONE,
      .borderColor = vk::BorderColor::eIntOpaqueBlack, //only useful when clamping
      .unnormalizedCoordinates = VK_FALSE, //Normalized coordinates allow to change texture resolution for the same UVs
    };

    s_baseColorSampler = vkTools::createSampler(samplerInfo, device);

    //Normal Map Sampler
    samplerInfo.anisotropyEnable = VK_FALSE; //Only bilinear filtering;

    s_normalSampler = vkTools::createSampler(samplerInfo, device);

    //Metallic Roughness Sampler
    s_metallicRoughnessSampler = vkTools::createSampler(samplerInfo, device);

    //Emissive sampler
    s_emissiveSampler = vkTools::createSampler(samplerInfo, device);
}

Material* Material::setBaseColor(const glm::vec4& color) {
    m_baseColorFactor = color;
    return this;
}

Material* Material::setEmissiveFactor(const glm::vec3& factor) {
    m_emissiveFactor = factor;
    return this;
}

Material* Material::setMetallicFactor(float factor) {
    m_metallicFactor = factor;
    return this;
}

Material* Material::setRoughnessFactor(float factor) {
    m_roughnessFactor = factor;
    return this;
}

Material* Material::setAlphaMode(AlphaMode alphaMode) {
    m_alphaMode = alphaMode;
    return this;
}

Material* Material::setAlbedoTexture(VulkanImage* texture)
{
    if (m_hasAlbedoTexture)
    {
        delete m_albedoTexture;
    }
    m_albedoTexture = texture;
    m_hasAlbedoTexture = true;
    return this;
}

bool Material::hasAlbedoTexture()
{
    return m_hasAlbedoTexture;
}

bool Material::hasNormalTexture()
{
    return m_hasNormalTexture;
}

bool Material::hasMetallicRoughness()
{
    return m_hasMetallicRoughnessTexture;
}

bool Material::hasEmissiveTexture()
{
    return m_hasEmissiveTexture;
}

Material* Material::setMetallicRoughnessTexture(VulkanImage* texture)
{
    if (m_hasMetallicRoughnessTexture) {
        delete m_metallicRoughnessTexture;
    }
    m_metallicRoughnessTexture = texture;
    m_hasMetallicRoughnessTexture = true;
    return this;
}

Material* Material::setNormalTexture(VulkanImage* texture) {
    if (m_hasNormalTexture)
    {
        delete m_normalTexture;
    }
    m_normalTexture = texture;
    m_hasNormalTexture = true;
    return this;
}

Material* Material::setEmissiveTexture(VulkanImage* texture) {
    if (m_hasEmissiveTexture)
    {
        delete m_emissiveTexture;
    }
    m_emissiveTexture = texture;
    m_hasEmissiveTexture = true;
    return this;
}

Material* Material::setAlphaCutoff(float cutoff)
{
    m_alphaCutoff = cutoff;
    return this;
}

VulkanImage* Material::getAlbedoTexture() {
    if (m_hasAlbedoTexture)
    {
        return m_albedoTexture;
    }
    throw std::runtime_error("Tried to retrieve an albedo texture that doesn't exist");
}

VulkanImage* Material::getNormalTexture() {
    if (m_hasNormalTexture) 
    {
        return m_normalTexture;
    }
    throw std::runtime_error("Tried to retrieve a normal texture that doesn't exist");
}

VulkanImage* Material::getMetallicRoughnessTexture() {
    if (m_hasMetallicRoughnessTexture) {
        return m_metallicRoughnessTexture;
    }
    throw std::runtime_error("Tried to retrieve a metallic roughness texture that doesn't exist");
}

VulkanImage* Material::getEmissiveTexture() {
    if (m_hasEmissiveTexture) {
        return m_emissiveTexture;
    }
    throw std::runtime_error("Tried to retrieve an emissive texture that doesn't exist");
}

MaterialUBO Material::getUBO() {
    return MaterialUBO{
        .baseColor = m_baseColorFactor,
        .emissiveColor = glm::vec4(m_emissiveFactor, 1.f),
        .metallicFactor = m_metallicFactor,
        .roughnessFactor = m_roughnessFactor,
        .alphaMode = static_cast<glm::uint>(m_alphaMode),
        .alphaCutoff = m_alphaCutoff,
        .hasAlbedoTexture = m_hasAlbedoTexture,
        .hasNormalTexture = m_hasNormalTexture,
        .hasMetallicRoughnessTexture = m_hasMetallicRoughnessTexture,
        .hasEmissiveTexture = m_hasEmissiveTexture,
        .albedoTextureId = m_albedoTextureId,
        .normalTextureId = m_normalTextureId,
        .metallicRoughnessTextureId = m_metallicRoughnessTextureId,
        .emissiveTextureId = m_emissiveTextureId,
    };
}