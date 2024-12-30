#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 fragPosView;
layout(location = 4) in vec4 fragTangent;
layout(location = 5) flat in uint meshletId;
layout(location = 6) flat in uint shellId;
layout(location = 7) flat in uint shellCount;

#define SHADOW_CASCADE_COUNT 4

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

void main()
{

}