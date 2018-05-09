#pragma once

#include <vector>

//nclude "GLFW\glfw3.h"

#include "Common.h"
#include "ParticleSystem.h"

void bindLight(GLuint shaderProgram, std::vector<Light*> lights);

void renderEntity(Entity &entity, GLuint shaderProgram, glm::mat4 &worldToView, glm::mat4 &perspective, bool useLights);

void renderSkybox(Entity &skybox, GLuint secondSkyboxTexture, float interpolation, GLuint shaderProgram, glm::mat4 &worldToView, glm::mat4 &perspective);

void renderTerrain(Terrain &terrain, GLuint shaderProgram, glm::mat4 &worldToView, glm::mat4 &perspective);

void renderParticleSystem(ParticleSystem &particleSystem, GLuint shaderProgram, int uniformLocationsTransformation[], int uniformLocationsProgress[], glm::mat4 &worldToView, glm::mat4 &perspective);

GLuint getShader(std::string vertexShaderPath, std::string fragmentShaderPath);

GLuint createSkybox(std::string x1, std::string x2, std::string y1, std::string y2, std::string z1, std::string z2);