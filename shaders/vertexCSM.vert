#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

#define SHADOW_CASCADE_COUNT 4

layout(set = 0, binding = 0) uniform CascadeUniformObject {
	mat4[SHADOW_CASCADE_COUNT] cascadeViewProj;
	vec4 cascadeSplits;
}ubo;

layout( push_constant ) uniform constants
{
	mat4 model;
	int textureId;
	int normalMapId;
	float time;
	uint cascadeId;
} PushConstants;


void main(){
	vec4 positionWorld = PushConstants.model * vec4(inPosition, 1.0);
	gl_Position = ubo.cascadeViewProj[PushConstants.cascadeId] * positionWorld;
}