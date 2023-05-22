#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 viewPosition;
layout(location = 4) in vec4 lightViewPosition;

layout( push_constant ) uniform constants
{
	mat4 model;
	int textureId;
	float time;
	vec2 data;
} PushConstants;


layout(set = 0, binding = 1) uniform sampler2D texSampler[];
layout(set = 1, binding = 0) uniform sampler2D shadowTexSampler;

layout(location = 0) out vec4 outColor;

/* Constants */
vec4 lightColor = vec4(1.0, 1.0, .95, 2.0);
vec3 ambientColor = vec3(1.0, 1.0, 1.0);

float shininess = 5;
float ambientIntensity = .5;

// returns the ambient intensity when in shadow or 1 when in light
float readShadowMap(vec4 lightViewCoord, vec2 uvOffset, float bias){
	
	float dist = texture(shadowTexSampler, lightViewCoord.xy + uvOffset).r;
	if(dist < lightViewCoord.z - bias ){
		return ambientIntensity;
	}else {
		return 1;
	}
}

// Percentage Closer Filtering approach of shadow mapping. Returns a number between ambient intensity and 1, an attenuation factor due to shadowing
float filterPCF(vec4 lightViewCoord, float bias)
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
			shadowFactor += readShadowMap(lightViewCoord, vec2(dx*x, dy*y), bias);
			count++;
		}
	}
	return shadowFactor / count;
}

void main(){
	vec3 lightPosition = vec3(0, 4 * sin(PushConstants.time), 10);
	vec3 lightDirection = lightPosition; //Light Position is distance to 0, 0, 0 here

	vec4 textureColor = texture(texSampler[PushConstants.textureId], fragTexCoord);

	// Diffuse lighting
	vec3 lightColor = lightColor.xyz * lightColor.a;
	vec3 diffuse = lightColor * max(ambientIntensity, dot(normalize(fragNormal), normalize(lightDirection)));

	// Specular lighting
	vec3 viewDirection = normalize(viewPosition - fragPosWorld);
	vec3 halfWayDirection = normalize(-lightDirection + viewDirection);
	float specular = pow(max(dot(viewDirection, halfWayDirection), 0.0), shininess);
	vec3 specularColor = lightColor.xyz * specular;

	//Shadows
	float cosTheta = clamp(dot(normalize(lightDirection), normalize(fragNormal)), 0, 1);
	float bias = 0.0005*tan(0.5*acos(cosTheta));
	bias = clamp(bias, 0,0.01);
	float shadowFactor = filterPCF(lightViewPosition / lightViewPosition.w, bias);
	
	outColor = vec4((diffuse + specularColor) * textureColor.xyz * shadowFactor, textureColor.a);
}