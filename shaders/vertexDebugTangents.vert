#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;

layout(binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
}ubo;

layout( push_constant ) uniform constants
{
	mat4 model;
	mat4 data;
} PushConstants;


layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragTangent;


void main(){
	gl_Position = ubo.proj * ubo.view * PushConstants.model * vec4(inPosition,  1.0);
	fragTexCoord = inTexCoord;
	fragNormal = inNormal;
	fragTangent = inTangent;
}