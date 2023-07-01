#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#define SHADOW_CASCADE_COUNT 4
#define MAX_LIGHT_COUNT 10
#define TRUE 1
#define FALSE 0
#define DIRECTIONAL_LIGHT_TYPE 0
#define POINT_LIGHT_TYPE 1
#define SPOTLIGHT_LIGHT_TYPE 2

#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK 1
#define ALPHA_MODE_TRANSPARENT 2

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 fragPosView;
layout(location = 4) in vec4 fragTangent;

layout( push_constant ) uniform constants
{
	mat4 model;
	int materialId;
	uint cascadeId;
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
	uint shadowCaster;
};

layout(set = 0, binding = 1) uniform LightsUBO {
	Light lights[MAX_LIGHT_COUNT];
}lightsUbo;

layout(set = 0, binding = 2) uniform sampler2DArray shadowTexSampler;


layout(set = 0, binding = 3) uniform sampler2D texSampler[];

struct Material{
	vec4 baseColor;
	vec4 emissiveColor;
	float metallicFactor;
	float roughnessFactor;
	uint alphaMode;
	float alphaCutoff;
	uint hasAlbedoTexture;
	uint hasNormalTexture;
	uint hasMetallicRoughnessTexture;
	uint hasEmissiveTexture;
	uint albedoTextureId;
	uint normalTextureId;
	uint metallicRoughnessTextureId;
	uint emissiveTextureId;
};


layout(set = 1, binding = 0) uniform MaterialUbo {
	Material material;
}materialUbo[];

layout(location = 0) out vec4 outColor;




/* Constants */
vec3 ambientColor = vec3(1.0, 1.0, 1.0);
const vec3 black = vec3(0, 0, 0);
const vec3 dielectricF0 = vec3(0.04);

float specularPower = 30;
float ambientIntensity = .4;
float specularAttenuation = 0.8;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );



struct BRDFResult {
	vec3 diffuseShadowCaster;
	vec3 specularShadowCaster;
	vec3 diffuse;
	vec3 specular;
};

struct IntermediateDotProducts{
	float VdotH;
	float VdotN;
	float LdotN;
	float NdotH;
};

float clampedDot(vec4 x, vec4 y)
{
	return clamp(dot(x, y), 0.0, 1.0);
}

vec3 schlickFresnelReflectance(vec3 f0, float VdotH)
{
	return f0 + (1 - f0) * pow((1 - abs(VdotH)), 5);
}

float computeAttenuation(Light light, float distance)
{
	return 1.0f - smoothstep(light.range * 0.75, light.range, distance); 
}

float computeSpotCone(Light light, vec4 L)
{
	float minCos = cos(radians(light.spotlightHalfAngle));
	float maxCos = (1 + minCos)*0.5; 
	float cosAngle = dot(light.directionWorld, -L);
	return smoothstep(minCos, maxCos, cosAngle);
}

vec3 diffuseBRDF(vec3 F, vec3 diffuseColor)
{
	return (1 - F) * (1 / 3.1415) * diffuseColor;
}

float D_GGX(float alpha, float NdotH)
{
	float f = (NdotH * NdotH) * (alpha - 1.0) + 1.0;
	return alpha / (3.1415 * f * f);
}

// G / (4 * NdotL * NdotV)
float V_GGX(float alpha, IntermediateDotProducts intDot)
{
	float GGXV = intDot.LdotN * sqrt(intDot.VdotN * intDot.VdotN * (1.0 - alpha) + alpha);
	float GGXL = intDot.VdotN * sqrt(intDot.LdotN * intDot.LdotN * (1.0 - alpha) + alpha);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

vec3 specularBRDF(vec3 F, float alpha, IntermediateDotProducts intDot)
{
	return (F * D_GGX(alpha, intDot.NdotH) * V_GGX(alpha, intDot));
}

BRDFResult computeDirectionalLight(Light light, float alpha, vec3 f0, vec3 diffuseColor, vec4 V, vec4 fragPosWorld, vec4 N)
{
	BRDFResult result;
	result.diffuse = vec3(0.0);
	result.specular = vec3(0.0);
	result.diffuseShadowCaster = vec3(0.0);
	result.specularShadowCaster = vec3(0.0);
	
	vec4 L = normalize(-light.directionWorld);


		vec4 H = normalize(V + L);

		IntermediateDotProducts intDot;
		intDot.VdotH = clampedDot(V, H);
		intDot.VdotN = clampedDot(V, N);
		intDot.LdotN = clampedDot(L, N);
		intDot.NdotH = clampedDot(N, H);
	if(intDot.LdotN > 0.0 || intDot.VdotN > 0.0)
	{

		vec3 F = schlickFresnelReflectance(f0, intDot.VdotH);
		if(light.shadowCaster == TRUE)
		{
			result.diffuseShadowCaster = diffuseBRDF(F, diffuseColor) * light.intensity * intDot.LdotN;
			result.specularShadowCaster = specularBRDF(F, alpha, intDot) * light.intensity * intDot.LdotN;
		} else {
			result.diffuse = diffuseBRDF(F, diffuseColor) * light.intensity * intDot.LdotN;
			//TODO Optimize higher
			result.specular = specularBRDF(F, alpha, intDot) * light.intensity * intDot.LdotN;
		}
		
	}

	return result;
}

BRDFResult computePointLight(Light light, float alpha, vec3 f0, vec3 diffuseColor, vec4 V, vec4 fragPosWorld, vec4 N)
{
	
	BRDFResult result;
	result.diffuse = vec3(0.0);
	result.specular = vec3(0.0);

	vec4 L = light.positionWorld - fragPosWorld;
	float distance = length(L);
	L = L/distance;


	vec4 H = normalize(V + L);

	IntermediateDotProducts intDot;
	intDot.VdotH = clampedDot(V, H);
	intDot.VdotN = clampedDot(V, N);
	intDot.LdotN = clampedDot(L, N);
	intDot.NdotH = clampedDot(N, H);
	
	if(intDot.LdotN > 0.0 || intDot.VdotN > 0.0)
	{

		vec3 F = schlickFresnelReflectance(f0, intDot.VdotH);

		float attenuation = computeAttenuation(light, distance);
		result.diffuse = diffuseBRDF(F, diffuseColor) * attenuation * light.intensity * intDot.LdotN;
		result.specular = specularBRDF(F, alpha, intDot) * attenuation * light.intensity * intDot.LdotN;
	}
	
	return result;
}
BRDFResult computeSpotLight(Light light, float alpha, vec3 f0, vec3 diffuseColor, vec4 V, vec4 fragPosWorld, vec4 N)
{
	BRDFResult result;
	result.diffuse = vec3(0.0);
	result.specular = vec3(0.0);


	vec4 L = light.positionWorld - fragPosWorld;
	float distance = length(L);
	L = L/distance;

	vec4 H = normalize(V + L);

	IntermediateDotProducts intDot;
	intDot.VdotH = clampedDot(V, H);
	intDot.VdotN = clampedDot(V, N);
	intDot.LdotN = clampedDot(L, N);
	intDot.NdotH = clampedDot(N, H);
	if(intDot.LdotN > 0.0 || intDot.VdotN > 0.0)
	{
		vec3 F = schlickFresnelReflectance(f0, intDot.VdotH);

		float attenuation = computeAttenuation( light, distance );
		float spotIntensity = computeSpotCone( light, L );

		result.diffuse = diffuseBRDF(F, diffuseColor) * attenuation * spotIntensity * light.intensity * intDot.LdotN;
		result.specular = specularBRDF(F, alpha, intDot) * attenuation * spotIntensity * light.intensity * intDot.LdotN;
	}

	return result;
}

BRDFResult computeLighting(Light[MAX_LIGHT_COUNT] lights, Material material, vec4 cameraPos, vec4 fragPosWorld, vec4 N, vec3 albedo, float metallic, float roughness)
{
	vec4 V = normalize(cameraPos - fragPosWorld);
	
	vec3 diffuseColor = mix(albedo, black, metallic);
	vec3 f0 = mix(dielectricF0, albedo, metallic);
	float alpha = roughness * roughness;

	BRDFResult brdfResultSum;
	brdfResultSum.diffuse = vec3(0.f, 0.f, 0.f);
	brdfResultSum.specular = vec3(0.f, 0.f, 0.f);
	brdfResultSum.diffuseShadowCaster = vec3(0.f, 0.f, 0.f);
	brdfResultSum.specularShadowCaster = vec3(0.f, 0.f, 0.f);

	for(uint i = 0; i < MAX_LIGHT_COUNT; i++)
	{
		BRDFResult currentBRDFResult;
		currentBRDFResult.diffuse = vec3(0.f, 0.f, 0.f);
		currentBRDFResult.specular = vec3(0.f, 0.f, 0.f);
		currentBRDFResult.diffuseShadowCaster = vec3(0.f, 0.f, 0.f);
		currentBRDFResult.specularShadowCaster = vec3(0.f, 0.f, 0.f);



		if(lights[i].enabled == FALSE)continue;
		//range culling
		if(lights[i].type != DIRECTIONAL_LIGHT_TYPE && length(lights[i].positionWorld - fragPosWorld) > lights[i].range )continue;

		switch(lights[i].type)
		{
			case DIRECTIONAL_LIGHT_TYPE:
			{
				currentBRDFResult = computeDirectionalLight(lights[i], alpha, f0, diffuseColor, V, fragPosWorld, N);
				brdfResultSum.diffuseShadowCaster += currentBRDFResult.diffuseShadowCaster * lights[i].lightColor.rgb;
				brdfResultSum.specularShadowCaster += currentBRDFResult.specularShadowCaster * lights[i].lightColor.rgb;
				break;
			}
			case POINT_LIGHT_TYPE:
			{
				currentBRDFResult = computePointLight(lights[i], alpha, f0, diffuseColor, V, fragPosWorld, N);
				break;
			}
			case SPOTLIGHT_LIGHT_TYPE:
			{
				currentBRDFResult = computeSpotLight(lights[i], alpha, f0, diffuseColor, V, fragPosWorld, N);
				break;
			}
		}
		brdfResultSum.diffuse += currentBRDFResult.diffuse * lights[i].lightColor.rgb;
		brdfResultSum.specular += currentBRDFResult.specular * lights[i].lightColor.rgb;
		
	}
	return brdfResultSum;
}

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
				float interpolationValue = (fragPosView.z - generalUbo.cascadeSplits[index[0]] + (generalUbo.shadowMapsBlendWidth/2)) / generalUbo.shadowMapsBlendWidth;
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
	Material material = materialUbo[PushConstants.materialId].material;	

	/*ALBEDO*/
	vec4 albedo = material.baseColor;
	if(material.hasAlbedoTexture == TRUE)
	{
		albedo *= texture(texSampler[material.albedoTextureId], fragTexCoord);
	}

	/*ALPHA MODE*/
	//TODOTRANSPARENT
	if(material.alphaMode == ALPHA_MODE_MASK)
	{
		if(albedo.a < material.alphaCutoff)
		{
			discard;
		}
	}


	/*NORMALS*/
	vec3 normal = normalize(fragNormal);
	if(material.hasNormalTexture == TRUE)
	{
		vec3 tangent = normalize(fragTangent.xyz);	
		tangent = (tangent - dot(tangent, normal) * normal); //Gram Schmidt for orthogonal vectors
		vec3 bitangent = cross(normal, tangent.xyz) * fragTangent.w; //Handedness to make sure it is correct

		mat3 TBN = mat3(tangent, bitangent, normal);
		vec3 localNormal = texture(texSampler[material.normalTextureId], fragTexCoord).rgb;
		localNormal.y = 1 - localNormal.y;
		localNormal = 2* localNormal - 1;
		normal = normalize(TBN * localNormal);
	}
	
	/*METALLIC ROUGHNESS*/
	float metallic = material.metallicFactor;
	float roughness = material.roughnessFactor;
	if(material.hasMetallicRoughnessTexture == TRUE)
	{
		vec4 metallicRoughnessTexture = texture(texSampler[material.metallicRoughnessTextureId], fragTexCoord);
		metallic *= metallicRoughnessTexture.b;
		roughness *= metallicRoughnessTexture.g;
		roughness = max(roughness, 0.001);
	}

	uint cascadeIndex[2] = uint[2](0, 0);
	for(uint i = 0; i < SHADOW_CASCADE_COUNT - 1; ++i) {
		if(fragPosView.z + (generalUbo.shadowMapsBlendWidth / 2) < generalUbo.cascadeSplits[i]) {	
			cascadeIndex[0] = i + 1;
		}
		if(fragPosView.z - (generalUbo.shadowMapsBlendWidth / 2) < generalUbo.cascadeSplits[i]) {	
			cascadeIndex[1] = i + 1;
		}
	}
		//Shadows
	vec4 lightViewPosition[2] = vec4[2](
		biasMat * generalUbo.cascadeViewProj[cascadeIndex[0]] * vec4(fragPosWorld, 1.0),
		biasMat * generalUbo.cascadeViewProj[cascadeIndex[1]] * vec4(fragPosWorld, 1.0));	
	vec4 lightViewCoords[2] = vec4[2](lightViewPosition[0] / lightViewPosition[0].w, lightViewPosition[1] / lightViewPosition[1].w);
	float shadowFactor = filterPCF(lightViewCoords , cascadeIndex);
	
	
	BRDFResult lightResult = computeLighting(lightsUbo.lights, materialUbo[PushConstants.materialId].material, vec4(generalUbo.cameraPosition, 1.0), vec4(fragPosWorld, 1.f), vec4(normal, 0.f), albedo.rgb, metallic, roughness);

	vec3 ambientResult = ambientColor * albedo.rgb * ambientIntensity;

	outColor = mix(vec4(shadowFactor * (lightResult.diffuse + lightResult.specular + ambientResult), albedo.a),
	vec4(lightResult.diffuse + lightResult.diffuseShadowCaster + lightResult.specular + lightResult.specularShadowCaster + ambientResult * (shadowFactor), albedo.a),
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