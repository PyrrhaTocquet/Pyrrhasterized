#version 450
#extension GL_EXT_nonuniform_qualifier : enable


layout( push_constant ) uniform constants
{
	mat4 model;
	int textureId;
	float time;
	vec2 data;
} PushConstants;


layout(set = 0, binding = 1) uniform sampler2D texSampler[];

void main(){

}