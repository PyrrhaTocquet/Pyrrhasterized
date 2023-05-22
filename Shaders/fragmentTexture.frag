#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 viewPosition;

layout( push_constant ) uniform constants
{
	mat4 model;
	int textureId;
	float time;
	vec2 data;
} PushConstants;


layout(binding = 1) uniform sampler2D texSampler[];

layout(location = 0) out vec4 outColor;


vec4 lightColor = vec4(1.0, 1.0, .95, 20.0);
vec3 ambientColor = vec3(1.0, 1.0, 1.0);

float shininess = 10;
float ambientIntensity = 5;
float fresnelExponent = 4;

void main(){
	vec3 lightPosition = vec3(.2, sin(PushConstants.time), 10);

	vec3 lightDirection = lightPosition - fragPosWorld;
	float attenuation = 1.0 / dot(lightDirection, lightDirection);
	vec4 textureColor = texture(texSampler[PushConstants.textureId], fragTexCoord);

	// Diffuse lighting
	vec3 lightColor = lightColor.xyz * lightColor.a * attenuation;
	vec3 diffuse = lightColor * max(ambientIntensity, dot(normalize(fragNormal), normalize(lightDirection)));

	// Specular lighting
	vec3 viewDirection = normalize(viewPosition - fragPosWorld);
	//vec3 reflectionDirection = reflect(-normalize(lightDirection), fragNormal);

	// Ambient Lighting

	vec3 halfWayDirection = normalize(-lightDirection + viewDirection);
	float specular = pow(max(dot(viewDirection, halfWayDirection), 0.0), shininess);
	vec3 specularColor = lightColor.xyz * specular;
	
	outColor = vec4((diffuse + specularColor) * textureColor.xyz , textureColor.a);
	
}