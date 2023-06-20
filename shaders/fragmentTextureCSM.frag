#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#define SHADOW_CASCADE_COUNT 4
#define MAX_LIGHT_COUNT 10

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
	uint cascadeId;
	float[13] padding;
} PushConstants;

layout(set = 0, binding = 0) uniform CameraGeneralUbo {
	mat4 view;
	mat4 proj;
	mat4[SHADOW_CASCADE_COUNT] cascadeViewProj;
	vec4 cascadeSplits;
	vec3 cameraPosition;
	float shadowMapsBlendWidth;
	float time;
}generalUbo;


struct Light{
	vec4 positionWorld;
	vec4 directionWorld;
	vec4 positionView;
	vec4 directionView;
	vec4 lightColor;
	float spotlightHalfAngle;
	float range;
	float intensity;
	uint enabled;
	uint type;
};

layout(set = 0, binding = 1) uniform LightsUBO {
	Light lights[MAX_LIGHT_COUNT];
}lightsUbo;



layout(set = 0, binding = 2) uniform sampler2D texSampler[];
layout(set = 1, binding = 0) uniform sampler2DArray shadowTexSampler;

layout(location = 0) out vec4 outColor;

/* Constants */
vec4 lightColor = vec4(1.0, 1.0, .95, 1.5);
vec3 ambientColor = vec3(1.0, 1.0, 1.0);

float shininess = 30;
float ambientIntensity = .4;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );


// returns the ambient intensity when in shadow or 1 when in light
float readShadowMap(vec4 lightViewCoord, vec2 uvOffset, uint index){
	
	float dist = texture(shadowTexSampler, vec3(lightViewCoord.xy + uvOffset, index)).r;
	if(dist < lightViewCoord.z ){
		return ambientIntensity;
	}else {
		return 1;
	}
}

// Percentage Closer Filtering approach of shadow mapping. Returns a number between ambient intensity and 1, an attenuation factor due to shadowing
float filterPCF(vec4[2] lightViewCoords, uint[2] index)
{
	ivec2 shadowMapDimensions = ivec2(4096, 4096);//textureSize(shadowTexSampler, 0);
	float scale = 1;
	float dx = scale * 1.0 / float(shadowMapDimensions.x);
	float dy = scale * 1.0 / float(shadowMapDimensions.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 3; //Averaging 16 samples

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			if(index[0] == index[1])
			{
				shadowFactor += readShadowMap(lightViewCoords[0], vec2(dx*x, dy*y), index[0]);
				count++;
			}else{
				float interpolationValue = (viewPosition.z - generalUbo.cascadeSplits[index[0]] + (generalUbo.shadowMapsBlendWidth/2)) / generalUbo.shadowMapsBlendWidth;
				shadowFactor += mix(
				readShadowMap(lightViewCoords[1], vec2(dx*x, dy*y), index[1]),
				readShadowMap(lightViewCoords[0], vec2(dx*x, dy*y), index[0]),
				interpolationValue);
				count++;
			}
		}
	}
	return shadowFactor / count;
}

void main(){

	vec4 textureColor = texture(texSampler[PushConstants.textureId], fragTexCoord);
	if(textureColor.a < 0.8)
	{
		discard;
	}

	vec3 tangent = normalize(fragTangent.xyz);
	vec3 normal = normalize(fragNormal);
	tangent = (tangent - dot(tangent, normal) * normal); //Gram Schmidt for orthogonal vectors
	vec3 bitangent = cross(normal, tangent.xyz) * fragTangent.w; //Handedness to make sure it is correct

	mat3 TBN = mat3(tangent, bitangent, normal);
	vec3 localNormal = texture(texSampler[PushConstants.normalMapId], fragTexCoord).rgb;
	localNormal.y = 1 - localNormal.y;
	localNormal = 2* localNormal - 1;
	normal = normalize(TBN * localNormal);


	uint cascadeIndex[2] = uint[2](0, 0);
	for(uint i = 0; i < SHADOW_CASCADE_COUNT - 1; ++i) {
		if(viewPosition.z + (generalUbo.shadowMapsBlendWidth / 2) < generalUbo.cascadeSplits[i]) {	
			cascadeIndex[0] = i + 1;
		}
		if(viewPosition.z - (generalUbo.shadowMapsBlendWidth / 2) < generalUbo.cascadeSplits[i]) {	
			cascadeIndex[1] = i + 1;
		}
	}
		//Shadows

	vec3 lightPosition = vec3(1.0, 50.f, 20.f * cos(generalUbo.time/8));
	//vec3 lightPosition = vec3(1.0, 50.f, 2.f);
	vec3 lightDirection = normalize(-lightPosition); //Light Position is distance to 0, 0, 0 here

	vec4 lightViewPosition[2] = vec4[2](
		biasMat * generalUbo.cascadeViewProj[cascadeIndex[0]] * vec4(fragPosWorld, 1.0),
		biasMat * generalUbo.cascadeViewProj[cascadeIndex[1]] * vec4(fragPosWorld, 1.0));	
	vec4 lightViewCoords[2] = vec4[2](lightViewPosition[0] / lightViewPosition[0].w, lightViewPosition[1] / lightViewPosition[1].w);
	float shadowFactor = filterPCF(lightViewCoords , cascadeIndex);
	
	

	// Diffuse lighting
	vec3 lightColor = lightColor.xyz * lightColor.a;
	vec3 diffuse = lightColor * max(ambientIntensity, dot(normal , -lightDirection));

	// Specular lighting
	vec3 viewDirection = normalize(generalUbo.cameraPosition - fragPosWorld);
	vec3 halfWayDirection = normalize(viewDirection - lightDirection);
	float specular = pow(max(dot(normal, halfWayDirection), 0.0), shininess);
	vec3 specularColor = lightColor.xyz * specular * 0.5;
		
	outColor = mix(vec4(shadowFactor * textureColor.xyz, textureColor.a),
	vec4((diffuse + specular) * textureColor.xyz * (shadowFactor + lightsUbo.lights[0].intensity), textureColor.a),
	(shadowFactor-ambientIntensity)/(1-ambientIntensity));
	
	

	/*switch(cascadeIndex[0]) {
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