/*
author: Pyrrha Tocquet
date: 01/04/25
desc: main material fragment shader. Does PBR lighting and shadows
*/
import "modules/global";
import "modules/geometry";
import "modules/light";
import "modules/shadows";
import "modules/material";

[[vk::binding(0, 1)]] ConstantBuffer<Global::GeneralUBO>                        generalUBO;
[[vk::binding(1, 1)]] ConstantBuffer<Light::LightData[Light::cMaxLightCount]>   lightsUBO;
[[vk::binding(2, 1)]] Sampler2DArray                                            shadowTexSampler;
[[vk::binding(3, 1)]] Sampler2D                                                 texturesTexSamplers[];

[[vk::binding(0, 2)]] ConstantBuffer<Material::MaterialData>                    materialUBO[];                                

[[shader("fragment")]]
void fragMain(
    in Geometry::MeshOutputVertex inVertex,
    in uniform Global::PushConstant inPushConstant,
    out float4 outColor: COLOR0 
)
{
    Material::MaterialData material = materialUBO[inPushConstant.materialId];

    /*ALBEDO*/
    float4 albedo = material.baseColor;
    if (material.hasAlbedoTexture == true)
    {
        albedo *= texturesTexSamplers[material.albedoTextureId].Sample(inVertex.texCoord);
    }

    /*ALPHA MODE*/
    // TODOTRANSPARENT
#ifdef SHELL_TEXTURING
    if (shellCount > 1)
    {
        vec2 newUV = inVertex.texCoord.xy * generalUBO.hairDensity;
        vec2 localUV = fract(newUV) * 2 - 1;
        float localDistanceFromCenter = length(localUV);

        uvec2 hashUV = uvec2(newUV);
        float disolveRandom = hash(hashUV.x + 100 * hashUV.y + 100 * 10) / 1.0;
        if (disolveRandom > (1 - shellId / float(shellCount)))
        {
            discard;
        }

        if (localDistanceFromCenter > (1 - shellId / float(shellCount)) && shellId != 0) {
            discard;
        }

        float halfLambert = 0.8 + 0.5 * dot(float3(-lightsUbo.lights[0].directionWorld), inVertex.normal);
        // outColor = float4(albedo.rgb * mix(.7, 1.0, (shellId + 1)/float(shellCount)), 1.0);
        outColor = float4(albedo.rgb * halfLambert, 1.0);
    } else 
    {
#endif
        if (material.alphaMode == Global::AlphaMode::Masked)
        {
            if (albedo.a < material.alphaCutoff)
            {
                discard;
            }
        }

        /*NORMALS*/
        float3 normal = normalize(inVertex.normal);
        if (material.hasNormalTexture == true)
        {
            float3 tangent = normalize(inVertex.tangent.xyz);
            tangent = (tangent - dot(tangent, normal) * normal);         // Gram Schmidt for orthogonal vectors
            float3 bitangent = cross(normal, tangent.xyz) * inVertex.tangent.w; // Handedness to make sure it is correct

            float3x3 TBN = float3x3(tangent, bitangent, normal);
            TBN = transpose(TBN);
            float3 localNormal = texturesTexSamplers[material.normalTextureId].Sample(inVertex.texCoord).rgb;
            localNormal.y = 1 - localNormal.y;
            localNormal = 2 * localNormal - 1;
            normal = normalize(mul(TBN, localNormal));
        }

        /*METALLIC ROUGHNESS*/
        float metallic = material.metallicFactor;
        float roughness = material.roughnessFactor;
        if (material.hasMetallicRoughnessTexture == true)
        {
            float4 metallicRoughnessTexture = texturesTexSamplers[material.metallicRoughnessTextureId].Sample(inVertex.texCoord);
            metallic *= metallicRoughnessTexture.b;
            roughness *= metallicRoughnessTexture.g;
            roughness = max(roughness, 0.001);
        }

        /*EMISSION*/
        float3 emissiveColor = material.emissiveColor.rgb;
        if (material.hasEmissiveTexture == true) {
            emissiveColor *= texturesTexSamplers[material.emissiveTextureId].Sample(inVertex.texCoord).rgb;
        }

        uint cascadeIndex[2] = uint[2](0, 0);
        for (uint i = 0; i < Shadows::cShadowCascadeCount - 1; ++i) {
            if (inVertex.posView.z + (generalUBO.shadowMapsBlendWidth / 2) < generalUBO.cascadeSplits[i]) {
                cascadeIndex[0] = i + 1;
            }
            if (inVertex.posView.z - (generalUBO.shadowMapsBlendWidth / 2) < generalUBO.cascadeSplits[i]) {
                cascadeIndex[1] = i + 1;
            }
        }
        // Shadows
        float4 lightViewPosition[2] = float4[2](
            mul(mul(Shadows::cBiasMat, generalUBO.cascadeViewProj[cascadeIndex[0]]), float4(inVertex.posWorld, 1.0)),
            mul(mul(Shadows::cBiasMat, generalUBO.cascadeViewProj[cascadeIndex[1]]), float4(inVertex.posWorld, 1.0)));
        float4 lightViewCoords[2] = float4[2](lightViewPosition[0] / lightViewPosition[0].w, lightViewPosition[1] / lightViewPosition[1].w);
        float shadowFactor = Shadows::filterPCF(inVertex.posView, generalUBO.cascadeSplits, generalUBO.shadowMapsBlendWidth, shadowTexSampler, lightViewCoords, cascadeIndex);

        Light::BRDFResult lightResult = Light::computeLighting(lightsUBO, materialUBO[inPushConstant.materialId], float4(generalUBO.cameraPosition, 1.f), float4(inVertex.posWorld, 1.f), float4(normal, 0.f), albedo.rgb, metallic, roughness);

        float3 ambientResult = Light::cAmbientColor * albedo.rgb * Global::cAmbientIntensity;

        float3 lightResultSum = lightResult.diffuse + lightResult.specular + ambientResult + emissiveColor;
        outColor = lerp(float4(shadowFactor * lightResultSum, albedo.a),
                       float4(lightResultSum + lightResult.diffuseShadowCaster + lightResult.specularShadowCaster * (shadowFactor), albedo.a),
                       (shadowFactor - Global::cAmbientIntensity) / (1 - Global::cAmbientIntensity));
}