#pragma once

Light createPointLight(glm::vec3 position, glm::vec3 scale, float intensity, float attenuationC1, float attenuationC2, glm::vec3 color);

Light createSpotlight(glm::vec3 position, glm::vec3 scale, float intensity, float attenuationC1, float attenuationC2, float cutoffAngle, glm::vec3 forward, glm::vec3 up, glm::vec3 color);

Light createDirectionalLight(glm::vec3 position, glm::vec3 scale, float intensity, glm::vec3 forward, glm::vec3 up, glm::vec3 color);
