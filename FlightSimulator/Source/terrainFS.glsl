#version 460

uniform sampler2D tex;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;

in vec3 lightDirectionVS;
in vec3 viewPositionVS;
in vec2 textureVS;
in vec3 normalVS;
in vec3 fragmentVS;

out vec4 gl_Color;

vec3 saturate(vec3 color, float saturate) {
    const vec3 w = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(color, w));
    return mix(intensity, color, saturate);
}

void main() {
	vec4 ambient = vec4(0.3,0.3,0.3,1);
	vec3 normal = normalize(normalVS);

	// Directional light
	vec3 fragToLight = -lightDirectionVS;
	float diffuse = dot(fragToLight, normal) * 1.5;
	vec4 diffuseColor = vec4(1, 1, 1, 1);
	
	// Specular highlight
	vec3 reflectionDirection = reflect(lightDirectionVS, normal);
	vec3 surfaceToCamera = normalize(viewPositionVS - fragmentVS);
	float cosAngle = max(0.0, dot(surfaceToCamera, reflectionDirection));
	float specularExponent = 2000;
	float specularCoefficient = pow(cosAngle, specularExponent);
	vec4 specularColor = vec4(0.5, 0.5, 0.6, 1);

	vec4 terrain1 = texture(tex, textureVS);
	vec4 terrain2 = texture(tex2, textureVS);
	vec4 terrain3 = texture(tex3, textureVS);
	vec4 splat = texture(tex4, textureVS / 40);
	gl_Color = (splat.x * terrain1 + splat.y * terrain2 + splat.z * terrain3) * diffuse + specularColor * specularCoefficient;

	// Fade far away objects
	// float camDistance = length(viewPositionVS - fragmentVS);
	// float distanceStartFade = 100;
	// float distanceEndFade = 1000;
	// if (camDistance > distanceStartFade && !isSkybox) {
	//	float saturation = clamp((camDistance - distanceStartFade) / (distanceEndFade - distanceStartFade), 0, 1);
	//	gl_Color = vec4(saturate(gl_Color.xyz, 1 - saturation), 1);
	//	gl_Color = mix(gl_Color, vec4(0.27, 0.43, 0.66, 1), saturation);
	// }
	
	gl_Color.w = 1;
}
