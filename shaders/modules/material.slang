/*
author: Pyrrha Tocquet
date: 01/04/25
desc: module for PBR material structs and functions
*/
module material;

namespace Material
{
    public struct MaterialData 
    {
        public float4 baseColor;
        public float4 emissiveColor;
        public float metallicFactor;
        public float roughnessFactor;
        public uint alphaMode;
        public float alphaCutoff;
        public uint hasAlbedoTexture; // convert to booleans ?
        public uint hasNormalTexture;
        public uint hasMetallicRoughnessTexture;
        public uint hasEmissiveTexture;
        public uint albedoTextureId;
        public uint normalTextureId;
        public uint metallicRoughnessTextureId;
        public uint emissiveTextureId;
    };
}

