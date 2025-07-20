/*
author: Pyrrha Tocquet
date: 01/04/25
desc: Contains the geometry related structs and functions
*/
module geometry;

namespace Geometry
{
    public struct InputVertex
    {
        public float3 pos;
        public float2 texCoord;
        public float3 normal;
        public float4 tangent; // w: handles handedness
    };

    public struct MeshOutputVertex
    {
        public float4 pos : SV_Position;
        public float2 texCoord : TEXCOORD0;
        public float3 normal : NORMAL0;
        public float3 posWorld : TEXCOORD1;
        public float3 posView : TEXCOORD2;
        public float4 tangent : TEXCOORD3;
        public nointerpolation uint meshletId : TEXCOORD4;
#ifdef SHELL_TEXTURING
        public nointerpolation uint shellId : TEXCOORD5;
        public nointerpolation uint shellCount : TEXCOORD6;
#endif
    }

    public struct MinimalOutputVertex
    {
        public float4 pos : SV_Position;
    }

    public struct MeshletInfo
    {
        public float4 boundingSphere;
        public uint vertexCount;
        public uint vertexOffset;
        public uint primitiveCount;
        public uint primitiveOffset;
        public uint meshletId; // TODO do we need this ???
    };

    public struct Triangle
    {
        public uint i0;
        public uint i1;
        public uint i2;
    };
}
