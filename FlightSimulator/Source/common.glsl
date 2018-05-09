#version 460

struct Light {
	int type; // 0 = directional, 1 = point light, 2 = spotlight
	vec3 position;
	vec3 direction; // For directional light and spotlight
	float cutoffAngle; // For spotlight, in radians
	float intensity; // For directional light since they don't use attenuation
	vec3 color;
	float attenuationC1;
	float attenuationC2;
};

bool isDirectional(Light light) {
	return light.type == 0;
}

bool isPoint(Light light) {
	return light.type == 1;
}

bool isSpot(Light light) {
	return light.type == 2;
}

// diffuse = diffuse component of material
// specilar = specular component of material
vec4 calculateLight(Light light, vec3 diffuse, vec3 specular, vec3 fragment, vec3 normal, vec3 viewPosition) {
	if (light.type == -1) {
		return vec4(0, 0, 0, 0);
	}
	vec3 lightToFragment = normalize(fragment - light.position);
	float attenuation = 1;
	if (isDirectional(light)) {
		lightToFragment = light.direction;
	}

	if (isPoint(light) || isSpot(light)) {
		float lightToFragmentDistance = length(light.position - fragment);
		attenuation = 1 / (1.0 + light.attenuationC1 * lightToFragmentDistance + light.attenuationC1 * lightToFragmentDistance * lightToFragmentDistance);
	}

	if (isSpot(light)) {
		float lambda = acos(dot(lightToFragment, light.direction)); // Current fragment "angle"
		float outerCutoffAngle = light.cutoffAngle + 0.02;
		if (lambda >= light.cutoffAngle) {
			attenuation = (outerCutoffAngle - lambda) / (outerCutoffAngle - light.cutoffAngle) * attenuation;
		}
		if (lambda > outerCutoffAngle) {
			attenuation = 0;
		}
	}

	vec3 fragmentToLight = -lightToFragment;
	vec3 fragmentToView = normalize(viewPosition - fragment);
	float diffuseIntensity = dot(fragmentToLight, normalize(normal)) * attenuation * light.intensity;
	
	vec3 reflectionDirection = reflect(lightToFragment, normalize(normal));
	float cosAngle = max(0.0, dot(fragmentToView, reflectionDirection));
	float specularExponent = 280f;
	float specularIntensity = pow(cosAngle, specularExponent) * attenuation * light.intensity;

	vec3 result = diffuseIntensity * light.color * diffuse + specularIntensity * light.color * specular;

	result = clamp(result, 0, 1);

	return vec4(result, 1);
}