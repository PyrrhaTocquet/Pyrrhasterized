#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#define SHADOW_CASCADE_COUNT 4

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 viewPosition;
layout(location = 4) in vec4 fragTangent;

layout( push_constant ) uniform constants
{
	mat4 model;
	int textureId;
	int normalMapId;
	float time;
	uint cascadeId;
	float[12] padding;
} PushConstants;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4[SHADOW_CASCADE_COUNT] cascadeViewProj;
	vec4 cascadeSplits;
	uint currentFrame;
	vec3 padding;
}ubo;


layout(set = 0, binding = 1) uniform sampler2D texSampler[];
layout(set = 1, binding = 0) uniform sampler2DArray shadowTexSampler;

layout(location = 0) out vec4 outColor;

/* Constants */
vec4 lightColor = vec4(1.0, 1.0, .95, 3.0);
vec3 ambientColor = vec3(1.0, 1.0, 1.0);

float shininess = 5000;
float ambientIntensity = .4;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

// returns the ambient intensity when in shadow or 1 when in light
float readShadowMap(vec4 lightViewCoord, vec2 uvOffset, float bias, uint index){
	
	float dist = texture(shadowTexSampler, vec3(lightViewCoord.xy + uvOffset, index)).r;
	if(dist < lightViewCoord.z - bias ){
		return ambientIntensity;
	}else {
		return 1;
	}
}

// Percentage Closer Filtering approach of shadow mapping. Returns a number between ambient intensity and 1, an attenuation factor due to shadowing
float filterPCF(vec4 lightViewCoord, float bias, uint index)
{
	ivec2 shadowMapDimensions = ivec2(4096, 4096);//textureSize(shadowTexSampler, 0);
	float scale = 1.0;
	float dx = scale * 1.0 / float(shadowMapDimensions.x);
	float dy = scale * 1.0 / float(shadowMapDimensions.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1; //Averaging 9 samples

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += readShadowMap(lightViewCoord, vec2(dx*x, dy*y), bias, index);
			count++;
		}
	}
	return shadowFactor / count;
}

void main(){

	vec4 textureColor = texture(texSampler[PushConstants.textureId], fragTexCoord);
	/*if(textureColor.a < 0.5)
	{
		discard;
	}*/

	vec3 tangent = normalize(fragTangent.xyz);
	vec3 normal = normalize(fragNormal);
	tangent = (tangent - dot(tangent, normal) * normal); //Gram Schmidt for orthogonal vectors
	vec3 bitangent = cross(normal, tangent.xyz) * fragTangent.w; //Handedness to make sure it is correct

	mat3 TBN = mat3(tangent, bitangent, normal);
	vec3 localNormal = texture(texSampler[PushConstants.normalMapId], fragTexCoord).rgb;
	localNormal.y = 1 - localNormal.y;
	localNormal = 2* localNormal - 1;
	normal = normalize(TBN * localNormal);


	uint cascadeIndex = 0;
	for(uint i = 0; i < SHADOW_CASCADE_COUNT - 1; ++i) {
		if(viewPosition.z < ubo.cascadeSplits[i]) {	
			cascadeIndex = i + 1;
		}
	}
		//Shadows

	vec3 lightPosition = vec3(0.0, 50.f, 10.f);
	vec3 lightDirection = normalize(-lightPosition); //Light Position is distance to 0, 0, 0 here


	float cosTheta = clamp(dot(-lightDirection, normalize(normal)), 0, 1);
	float bias = 0.001*tan(0.5*acos(cosTheta));
	bias = clamp(bias, 0, 0.01);

	vec4 lightViewPosition = (biasMat * ubo.cascadeViewProj[cascadeIndex]) * vec4(fragPosWorld, 1.0);	
	float shadowFactor = readShadowMap(lightViewPosition / lightViewPosition.w, vec2(0,0) ,bias, ubo.currentFrame * 4 + cascadeIndex);
	
	


	// Diffuse lighting
	vec3 lightColor = lightColor.xyz * lightColor.a;
	vec3 diffuse = lightColor * max(ambientIntensity, dot(normal , normalize(lightDirection)));

	// Specular lighting
	vec3 viewDirection = normalize(viewPosition - fragPosWorld);
	vec3 halfWayDirection = normalize(lightDirection + viewDirection);
	float specular = pow(max(dot(viewDirection, halfWayDirection), 0.0), shininess);
	vec3 specularColor = lightColor.xyz * specular;
	
	
	outColor = vec4((diffuse + specular) * textureColor.xyz * shadowFactor, textureColor.a);



	/*switch(cascadeIndex) {
			case 0 : 
				outColor *= vec4(1.0f, 0.25f, 0.25f, 1.f);
				break;
			case 1 : 
				outColor *= vec4(0.25f, 1.0f, 0.25f, 1.f);
				break;
			case 2 : 
				outColor *= vec4(0.25f, 0.25f, 1.0f, 1.f);
				break;
			case 3 : 
				outColor *= vec4(1.0f, 1.0f, 0.25f, 1.f);
				break;
	}*/
}