#include "Common.h"
#include <glm/gtx/transform.hpp> 

Light createPointLight(glm::vec3 position, glm::vec3 scale, float intensity, float attenuationC1, float attenuationC2, glm::vec3 color) {
	Light light = Light();
	light.lightType = LightType::POINT_LIGHT;
	light.attenuationC1 = attenuationC1;
	light.attenuationC2 = attenuationC2;
	light.color = color;
	light.intensity = 3;
	Model lightModel = getVAOCube();
	lightModel.color = glm::vec4(light.color.x, light.color.y, light.color.z, 1);
	light.setModel(lightModel);
	light.position = position;
	light.scale = scale;
	return light;
}

Light createSpotlight(glm::vec3 position, glm::vec3 scale, float intensity, float attenuationC1, float attenuationC2, float cutoffAngle, glm::vec3 forward, glm::vec3 up, glm::vec3 color) {
	Light light = Light();
	light.lightType = LightType::SPOTLIGHT;
	light.cutoffAngle = cutoffAngle;
	light.attenuationC1 = attenuationC1;
	light.attenuationC2 = attenuationC2;
	light.forward = glm::normalize(forward);
	light.up = glm::normalize(up);
	light.color = color;
	light.intensity = intensity;
	Model lightModel = getVAOCube();
	lightModel.color = glm::vec4(light.color.x, light.color.y, light.color.z, 1);
	light.setModel(lightModel);
	light.position = position;
	light.scale = scale;
	return light;
}

Light createDirectionalLight(glm::vec3 position, glm::vec3 scale, float intensity, glm::vec3 forward, glm::vec3 up, glm::vec3 color) {
	Light light = Light();
	light.lightType = LightType::DIRECTIONAL_LIGHT;
	light.forward = glm::normalize(glm::vec3(0, -1, 0));
	light.up = glm::normalize(glm::vec3(0, 0, 1));
	light.color = color;
	light.intensity = intensity;
	Model lighttModel = getVAOCube();
	lighttModel.color = glm::vec4(light.color.x, light.color.y, light.color.z, 1);
	light.setModel(lighttModel);
	light.position = position;
	light.scale = scale;
	return light;
}
