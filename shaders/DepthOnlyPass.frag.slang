/*
author: Pyrrha Tocquet
date: 07/04/2025
desc: Depth Pre-Pass fragment shader
*/
import "modules/global";
import "modules/geometry";
import "modules/shadows";
import "modules/material";

[[vk::binding(0, 1)]] ConstantBuffer<Global::GeneralUBO> generalUbo;
[[vk::binding(1, 1)]] Sampler2D texSampler[];
[[vk::binding(0, 2)]] ConstantBuffer<Material::MaterialData> materialUbo[];

[[shader("fragment")]]
void fragMain(
    in Geometry::MeshOutputVertex inVertex,
    in uniform Global::PushConstant inPushConstant
)
{
    Material::MaterialData material = materialUbo[inPushConstant.materialId];

    /*ALBEDO*/
    float4 albedo = material.baseColor;
    if (material.hasAlbedoTexture == true)
    {
        albedo *= texSampler[material.albedoTextureId].Sample(inVertex.texCoord);
    }

    if (material.alphaMode == Global::AlphaMode::Masked)
    {
        if (albedo.a < material.alphaCutoff)
        {
            discard;
        }
    }
}