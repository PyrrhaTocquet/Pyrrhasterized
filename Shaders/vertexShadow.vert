#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4 lightView;
	mat4 lightProj;
}ubo;

layout( push_constant ) uniform constants
{
	mat4 model;
	mat4 data;
} PushConstants;


void main(){
	vec4 positionWorld = PushConstants.model * vec4(inPosition, 1.0);
	gl_Position = ubo.lightProj * ubo.lightView * positionWorld;
}