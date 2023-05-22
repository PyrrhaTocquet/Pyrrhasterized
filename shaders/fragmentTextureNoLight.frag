#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;

layout( push_constant ) uniform constants
{
	mat4 model;
	int textureId;
	float time;
	vec2 data;
} PushConstants;


layout(binding = 1) uniform sampler2D texSampler[];

layout(location = 0) out vec4 outColor;

void main(){
	outColor = texture(texSampler[PushConstants.textureId], fragTexCoord);
}