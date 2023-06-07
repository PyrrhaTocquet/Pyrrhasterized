#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4 lightView;
	mat4 lightProj;
	vec4 cascadeSplits;
}ubo;

layout( push_constant ) uniform constants
{
	mat4 model;
	mat4 data;
} PushConstants;


layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 viewPosition;
layout(location = 4) out vec4 lightViewPosition;
layout(location = 5) out vec4 fragTangent;
layout(location = 6) out float viewDepth;


const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );



void main(){
	vec4 positionWorld = PushConstants.model * vec4(inPosition, 1.0);

	fragTangent = vec4(normalize(mat3(PushConstants.model) * inTangent.xyz), inTangent.w);  
	fragTexCoord = inTexCoord;
	fragNormal = mat3(PushConstants.model) * inNormal;
	fragPosWorld = positionWorld.xyz;
	viewPosition = (ubo.view * positionWorld).xyz;
	lightViewPosition = biasMat * ubo.lightProj * ubo.lightView * positionWorld;

	gl_Position = ubo.proj * ubo.view * positionWorld;
}