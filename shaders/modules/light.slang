/*
author: Pyrrha Tocquet
date: 01/04/25
desc: module for PBR lightning structs and functions
*/
module light;
import "global";
import "material";

namespace Light
{
    public static const uint cMaxLightCount = 10u;
    public static const float3 cAmbientColor = float3(1.f, 1.f, 1.f);
    static const float3 cBlack = float3(0.f, 0.f, 0.f);
    static const float3 cDielectricF0 = float3(0.04f);

    static const float cSpecularPower = 30.f;
    static float cSpecularAttenuation = 0.8f;


    public enum LightTypes
    {
        DirectionalLight,
        PointLight,
        Spotlight
    }

    public struct LightData {
        public float4 positionWorld;
        public float4 directionWorld;
        public float4 positionView;
        public float4 directionView;
        public float4 lightColor;
        public float spotlightHalfAngle;
        public float range;
        public float intensity;
        public uint enabled; // make it a bool ?
        public uint type;
        public uint shadowCaster; // TODO make it a bool ?
    };

    public struct BRDFResult // TODO Does it need to be public ? 
    {
        public float3 diffuseShadowCaster;
        public float3 specularShadowCaster;
        public float3 diffuse;
        public float3 specular;
    };

    struct IntermediateDotProducts {
        float VdotH;
        float VdotN;
        float LdotN;
        float NdotH;
    };

    // Functions
    float clampedDot(float4 x, float4 y)
    {
        return clamp(dot(x, y), 0.0, 1.0);
    }

    float3 schlickFresnelReflectance(float3 f0, float VdotH)
    {
        return f0 + (1 - f0) * pow((1 - abs(VdotH)), 5);
    }

    float computeAttenuation(LightData light, float distance)
    {
        return 1.0f - smoothstep(light.range * 0.75, light.range, distance);
    }

    float computeSpotCone(LightData light, float4 L)
    {
        float minCos = cos(radians(light.spotlightHalfAngle));
        float maxCos = (1 + minCos) * 0.5;
        float cosAngle = dot(light.directionWorld, -L);
        return smoothstep(minCos, maxCos, cosAngle);
    }

    float3 diffuseBRDF(float3 F, float3 diffuseColor)
    {
        return (1 - F) * (1 / 3.1415) * diffuseColor;
    }

    float D_GGX(float alpha, float NdotH)
    {
        float f = (NdotH * NdotH) * (alpha - 1.0) + 1.0;
        return alpha / (3.1415 * f * f);
    }

    // G / (4 * NdotL * NdotV)
    float V_GGX(float alpha, IntermediateDotProducts intDot)
    {
        float GGXV = intDot.LdotN * sqrt(intDot.VdotN * intDot.VdotN * (1.0 - alpha) + alpha);
        float GGXL = intDot.VdotN * sqrt(intDot.LdotN * intDot.LdotN * (1.0 - alpha) + alpha);

        float GGX = GGXV + GGXL;
        if (GGX > 0.0)
        {
            return 0.5 / GGX;
        }
        return 0.0;
    }

    float3 specularBRDF(float3 F, float alpha, IntermediateDotProducts intDot)
    {
        return (F * D_GGX(alpha, intDot.NdotH) * V_GGX(alpha, intDot));
    }

    BRDFResult computeDirectionalLight(LightData light, float alpha, float3 f0, float3 diffuseColor, float4 V, float4 fragPosWorld, float4 N)
    {
        BRDFResult result;
        result.diffuse = float3(0.0);
        result.specular = float3(0.0);
        result.diffuseShadowCaster = float3(0.0);
        result.specularShadowCaster = float3(0.0);

        float4 L = normalize(-light.directionWorld);

        float4 H = normalize(V + L);

        IntermediateDotProducts intDot;
        intDot.VdotH = clampedDot(V, H);
        intDot.VdotN = clampedDot(V, N);
        intDot.LdotN = clampedDot(L, N);
        intDot.NdotH = clampedDot(N, H);
        if (intDot.LdotN > 0.0 || intDot.VdotN > 0.0)
        {

            float3 F = schlickFresnelReflectance(f0, intDot.VdotH);
            if (light.shadowCaster == true)
            {
                result.diffuseShadowCaster = diffuseBRDF(F, diffuseColor) * light.intensity * intDot.LdotN;
                result.specularShadowCaster = specularBRDF(F, alpha, intDot) * light.intensity * intDot.LdotN;
            } else {
                result.diffuse = diffuseBRDF(F, diffuseColor) * light.intensity * intDot.LdotN;
                // TODO Optimize higher
                result.specular = specularBRDF(F, alpha, intDot) * light.intensity * intDot.LdotN;
            }
        }

        return result;
    }

    BRDFResult computePointLight(LightData light, float alpha, float3 f0, float3 diffuseColor, float4 V, float4 fragPosWorld, float4 N)
    {

        BRDFResult result;
        result.diffuse = float3(0.0);
        result.specular = float3(0.0);

        float4 L = light.positionWorld - fragPosWorld;
        float distance = length(L);
        L = L / distance;

        float4 H = normalize(V + L);

        IntermediateDotProducts intDot;
        intDot.VdotH = clampedDot(V, H);
        intDot.VdotN = clampedDot(V, N);
        intDot.LdotN = clampedDot(L, N);
        intDot.NdotH = clampedDot(N, H);

        if (intDot.LdotN > 0.0 || intDot.VdotN > 0.0)
        {

            float3 F = schlickFresnelReflectance(f0, intDot.VdotH);

            float attenuation = computeAttenuation(light, distance);
            result.diffuse = diffuseBRDF(F, diffuseColor) * attenuation * light.intensity * intDot.LdotN;
            result.specular = specularBRDF(F, alpha, intDot) * attenuation * light.intensity * intDot.LdotN;
        }

        return result;
    }
    BRDFResult computeSpotLight(LightData light, float alpha, float3 f0, float3 diffuseColor, float4 V, float4 fragPosWorld, float4 N)
    {
        BRDFResult result;
        result.diffuse = float3(0.0);
        result.specular = float3(0.0);

        float4 L = light.positionWorld - fragPosWorld;
        float distance = length(L);
        L = L / distance;

        float4 H = normalize(V + L);

        IntermediateDotProducts intDot;
        intDot.VdotH = clampedDot(V, H);
        intDot.VdotN = clampedDot(V, N);
        intDot.LdotN = clampedDot(L, N);
        intDot.NdotH = clampedDot(N, H);
        if (intDot.LdotN > 0.0 || intDot.VdotN > 0.0)
        {
            float3 F = schlickFresnelReflectance(f0, intDot.VdotH);

            float attenuation = computeAttenuation(light, distance);
            float spotIntensity = computeSpotCone(light, L);

            result.diffuse = diffuseBRDF(F, diffuseColor) * attenuation * spotIntensity * light.intensity * intDot.LdotN;
            result.specular = specularBRDF(F, alpha, intDot) * attenuation * spotIntensity * light.intensity * intDot.LdotN;
        }

        return result;
    }

    public BRDFResult computeLighting(LightData[cMaxLightCount] lights, Material::MaterialData material, float4 cameraPos, float4 fragPosWorld, float4 N, float3 albedo, float metallic, float roughness)
    {
        float4 V = normalize(cameraPos - fragPosWorld);

        float3 diffuseColor = lerp(albedo, cBlack, metallic);
        float3 f0 = lerp(cDielectricF0, albedo, metallic);
        float alpha = roughness * roughness;

        BRDFResult brdfResultSum;
        brdfResultSum.diffuse = float3(0.f, 0.f, 0.f);
        brdfResultSum.specular = float3(0.f, 0.f, 0.f);
        brdfResultSum.diffuseShadowCaster = float3(0.f, 0.f, 0.f);
        brdfResultSum.specularShadowCaster = float3(0.f, 0.f, 0.f);

        for (uint i = 0; i < cMaxLightCount; i++)
        {
            BRDFResult currentBRDFResult;
            currentBRDFResult.diffuse = float3(0.f, 0.f, 0.f);
            currentBRDFResult.specular = float3(0.f, 0.f, 0.f);
            currentBRDFResult.diffuseShadowCaster = float3(0.f, 0.f, 0.f);
            currentBRDFResult.specularShadowCaster = float3(0.f, 0.f, 0.f);

            if (lights[i].enabled == false) continue;
            // range culling
            if (lights[i].type != LightTypes::DirectionalLight && length(lights[i].positionWorld - fragPosWorld) > lights[i].range) continue;

            switch (LightTypes(lights[i].type))
            {
            case LightTypes::DirectionalLight:
            {
                currentBRDFResult = computeDirectionalLight(lights[i], alpha, f0, diffuseColor, V, fragPosWorld, N);
                brdfResultSum.diffuseShadowCaster += currentBRDFResult.diffuseShadowCaster * lights[i].lightColor.rgb;
                brdfResultSum.specularShadowCaster += currentBRDFResult.specularShadowCaster * lights[i].lightColor.rgb;
                break;
            }
            case LightTypes::PointLight:
            {
                currentBRDFResult = computePointLight(lights[i], alpha, f0, diffuseColor, V, fragPosWorld, N);
                break;
            }
            case LightTypes::Spotlight:
            {
                currentBRDFResult = computeSpotLight(lights[i], alpha, f0, diffuseColor, V, fragPosWorld, N);
                break;
            }
            }
            brdfResultSum.diffuse += currentBRDFResult.diffuse * lights[i].lightColor.rgb;
            brdfResultSum.specular += currentBRDFResult.specular * lights[i].lightColor.rgb;
        }
        return brdfResultSum;
    }
}