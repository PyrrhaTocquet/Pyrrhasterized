#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;
#define SHADOW_CASCADE_COUNT 4
layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4[SHADOW_CASCADE_COUNT] cascadeViewProj;
	vec4 cascadeSplits;
	uint currentFrame;
	vec3 padding;
}ubo;

layout( push_constant ) uniform constants
{
	mat4 model;
	int textureId;
	int normalMapId;
	float time;
	uint cascadeId;
	float[12] padding;
} PushConstants;


layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 viewPosition;
layout(location = 4) out vec4 fragTangent;







void main(){
	vec4 positionWorld = PushConstants.model * vec4(inPosition, 1.0);

	fragTangent = vec4(normalize(mat3(PushConstants.model) * inTangent.xyz), inTangent.w);  
	fragTexCoord = inTexCoord;
	fragNormal = mat3(PushConstants.model) * inNormal;
	fragPosWorld = positionWorld.xyz;
	viewPosition = (ubo.view * positionWorld).xyz;

	gl_Position = ubo.proj * ubo.view * positionWorld;
}