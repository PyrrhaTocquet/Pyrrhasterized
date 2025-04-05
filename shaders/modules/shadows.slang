/*
author: Pyrrha Tocquet
date: 31/03/25
desc: Shadow maps related structs and functions
*/

module shadows;
import "global";



namespace Shadows
{
    public static const uint cShadowCascadeCount = 4;

    public static const float4x4 cBiasMat = float4x4( 
	0.5, 0.0, 0.0, 0.5,
	0.0, 0.5, 0.0, 0.5,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0 );

    public struct CascadeUniformObject
    {
        public float4x4 cascadeViewProj[cShadowCascadeCount];
        public float4 cascadeSplits;
    }

    // returns the ambient intensity when in shadow or 1 when in light
    public float readShadowMap(Sampler2DArray shadowTexSampler, float4 lightViewCoord, float2 uvOffset, uint index) {

        float dist = shadowTexSampler.Sample(float3(lightViewCoord.xy + uvOffset, index)).r;
        if (dist < lightViewCoord.z) {
            return Global::cAmbientIntensity;
        } else {
            return 1;
        }
    }

    // Percentage Closer Filtering approach of shadow mapping. Returns a number between ambient intensity and 1, an attenuation factor due to shadowing
    public float filterPCF(float3 viewPos, float4 cascadeSplits, float shadowMapsBlendWidth, Sampler2DArray shadowTexSampler, float4[2] lightViewCoords, uint[2] index)
    {
        int2 shadowMapDimensions = int2(4096, 4096); // textureSize(shadowTexSampler, 0);
        float scale = 1;
        float dx = scale * 1.0 / float(shadowMapDimensions.x);
        float dy = scale * 1.0 / float(shadowMapDimensions.y);

        float shadowFactor = 0.0;
        int count = 0;
        int range = 1; // Averaging 16 samples

        for (int x = -range; x <= range; x++)
        {
            for (int y = -range; y <= range; y++)
            {
                if (index[0] == index[1])
                {
                    shadowFactor += readShadowMap(shadowTexSampler, lightViewCoords[0], float2(dx * x, dy * y), index[0]);
                    count++;
                } else 
                {
                    float interpolationValue = (viewPos.z - cascadeSplits[index[0]] + (shadowMapsBlendWidth / 2.f)) / shadowMapsBlendWidth;
                    shadowFactor += lerp(
                        readShadowMap(shadowTexSampler, lightViewCoords[1], float2(dx * x, dy * y), index[1]),
                        readShadowMap(shadowTexSampler, lightViewCoords[0], float2(dx * x, dy * y), index[0]),
                        interpolationValue);
                    count++;
                }
            }
        }
        return shadowFactor / count;
    }

    float hash(uint x) {
        // integer hash copied from Hugo Elias
        x = (x << 13) ^ x;
        float t = float((x * (x * x * 15731u + 789221u) + 1376312589u) & 0x7fffffffu);
        return 1.0f - (t / 1073741824.0f);
    }
}
