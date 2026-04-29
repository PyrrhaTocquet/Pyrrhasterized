/*
author: Pyrrha Tocquet
date: 31/03/25
desc: Global structs and funcs accessible in many shader sources
*/

module global;

//#define SHELL_TEXURING

namespace Global
{
    public static const float cAmbientIntensity = .6f; //REFACTORIZE THIS I SWEAR ON FUCKING LAIN
    
    public static const float4 debugMeshletColor[10] = float4[10](
        float4(0.0, 0.0, 0.0, 1.0),
        float4(1.0, 0.0, 0.0, 1.0),
        float4(0.0, 1.0, 0.0, 1.0),
        float4(0.0, 0.0, 1.0, 1.0),
        float4(1.0, 1.0, 0.0, 1.0),
        float4(0.0, 1.0, 1.0, 1.0),
        float4(1.0, 0.0, 1.0, 1.0),
        float4(1.0, 1.0, 1.0, 1.0),
        float4(0.5, 0.5, 0.0, 1.0),
        float4(0.0, 0.5, 0.5, 1.0)
    );

    public struct GeneralUBO
    {
        public float4x4 view;
        public float4x4 proj;
        public float4x4 invProj;
        public float4x4[4] cascadeViewProj;
        public float4 cascadeSplits;
        public float3 cameraPosition;
        public float shadowMapsBlendWidth;
        public float2 screenRes;
        public float time;
        public float hairLength;
        public float gravityFactor;
        public float hairDensity;
    }

    public struct PushConstant
    {
        public float4x4 model;
        public int materialId;
        public uint cascadeId;
        public uint meshletId;
        public uint meshletCount;
    }

    public enum AlphaMode
    {
        Opaque,
        Masked,
        Transparent,
    }
}
