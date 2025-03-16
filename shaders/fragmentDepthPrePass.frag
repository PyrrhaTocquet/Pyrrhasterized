#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 fragPosView;
layout(location = 4) in vec4 fragTangent;
layout(location = 5) flat in uint meshletId;
layout(location = 6) flat in uint shellId;
layout(location = 7) flat in uint shellCount;

#define SHADOW_CASCADE_COUNT 4
#define TRUE 1
#define FALSE 0
#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK 1
#define ALPHA_MODE_TRANSPARENT 2

layout( push_constant ) uniform constants
{
	mat4 model;
	int materialId;
	uint cascadeId;
	uint meshletId;
} PushConstants;

layout(set = 1, binding = 0) uniform CameraGeneralUbo {
	mat4 view;
	mat4 proj;
	mat4[SHADOW_CASCADE_COUNT] cascadeViewProj;
	vec4 cascadeSplits;
	vec3 cameraPosition;
	float shadowMapsBlendWidth;
	float time;
	float hairLength;
	float gravityFactor;
	float hairDensity;
}generalUbo;

layout(set = 1, binding = 1) uniform sampler2D texSampler[];

struct Material{
	vec4 baseColor;
	vec4 emissiveColor;
	float metallicFactor;
	float roughnessFactor;
	uint alphaMode;
	float alphaCutoff;
	uint hasAlbedoTexture;
	uint hasNormalTexture;
	uint hasMetallicRoughnessTexture;
	uint hasEmissiveTexture;
	uint albedoTextureId;
	uint normalTextureId;
	uint metallicRoughnessTextureId;
	uint emissiveTextureId;
};


layout(set = 2, binding = 0) uniform MaterialUbo {
	Material material;
}materialUbo[];



void main()
{
	Material material = materialUbo[PushConstants.materialId].material;	

	/*ALBEDO*/
	vec4 albedo = material.baseColor;
	if(material.hasAlbedoTexture == TRUE)
	{
		albedo *= texture(texSampler[material.albedoTextureId], fragTexCoord);
	}

	if(material.alphaMode == ALPHA_MODE_MASK)
	{
		if(albedo.a < material.alphaCutoff)
		{
			discard;
		}
	}

}