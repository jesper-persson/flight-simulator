#include <iostream>
#include <vector>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtx/euler_angles.hpp> 
#include <glm/gtx/transform.hpp> 
#include <glm/gtx/rotate_vector.hpp> 
#include <glm/gtx/quaternion.hpp>

#include "ParticleSystem.h"
#include <lodepng.h>
#include "Common.h"
#include "DiamondSquare.h"
#include "Physics.h"
#include "EntityFactory.h"

const int NUM_PARTICLES_PER_DRAW_CALL = 100;

void bindLight(GLuint shaderProgram, std::vector<Light*> lights) {
	glUseProgram(shaderProgram);
	for (int i = 0; i < lights.size(); i++) {
		Light *light = lights[i];
		glm::mat4 transformation = getEntityTransformation(*light);
		glm::vec4 worldPosition = transformation * glm::vec4(0, 0, 0, 1);
		glm::vec4 worldForward = glm::normalize( transformation * glm::vec4(0, 0, 1, 0) );
		glUniform3f(glGetUniformLocation(shaderProgram, (std::string("lights[") + std::to_string(i) + std::string("].position")).c_str()), worldPosition.x, worldPosition.y, worldPosition.z);
		glUniform3f(glGetUniformLocation(shaderProgram, (std::string("lights[") + std::to_string(i) + std::string("].direction")).c_str()), worldForward.x, worldForward.y, worldForward.z);
		glUniform3f(glGetUniformLocation(shaderProgram, (std::string("lights[") + std::to_string(i) + std::string("].color")).c_str()), light->color.x, light->color.y, light->color.z);
		glUniform1f(glGetUniformLocation(shaderProgram, (std::string("lights[") + std::to_string(i) + std::string("].intensity")).c_str()), light->intensity);
		glUniform1i(glGetUniformLocation(shaderProgram, (std::string("lights[") + std::to_string(i) + std::string("].type")).c_str()), light->lightType);
		glUniform1f(glGetUniformLocation(shaderProgram, (std::string("lights[") + std::to_string(i) + std::string("].cutoffAngle")).c_str()), light->cutoffAngle);
		glUniform1f(glGetUniformLocation(shaderProgram, (std::string("lights[") + std::to_string(i) + std::string("].attenuationC1")).c_str()), light->attenuationC1);
		glUniform1f(glGetUniformLocation(shaderProgram, (std::string("lights[") + std::to_string(i) + std::string("].attenuationC2")).c_str()), light->attenuationC2);
		light++;
	}
}

void renderEntity(Entity &entity, GLuint shaderProgram, glm::mat4 &worldToView, glm::mat4 &perspective, bool useLights) {
	glUseProgram(shaderProgram);
	glm::mat4 transformation = getEntityTransformation(entity);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "worldToView"), 1, GL_FALSE, glm::value_ptr(worldToView));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(perspective));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelToWorld"), 1, GL_FALSE, glm::value_ptr(transformation));

	Model model = entity.getModel();
	glUniform3f(glGetUniformLocation(shaderProgram, "Ka"), entity.getModel().Ka.x, entity.getModel().Ka.y, entity.getModel().Ka.z);
	glUniform3f(glGetUniformLocation(shaderProgram, "Kd"), entity.getModel().Kd.x, entity.getModel().Kd.y, entity.getModel().Kd.z);
	glUniform3f(glGetUniformLocation(shaderProgram, "Ks"), entity.getModel().Ks.x, entity.getModel().Ks.y, entity.getModel().Ks.z);
	glUniform1f(glGetUniformLocation(shaderProgram, "specularExponent"), entity.getModel().Ns);
	glUniform1f(glGetUniformLocation(shaderProgram, "dissolve"), entity.getModel().d);
	glUniform4f(glGetUniformLocation(shaderProgram, "color"), model.color.x, model.color.y, model.color.z, model.color.w);

	glUniform1i(glGetUniformLocation(shaderProgram, "useLights"), useLights);

	glBindVertexArray(entity.getModel().vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, entity.textureId);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, entity.normalMapId);
	glUniform1i(glGetUniformLocation(shaderProgram, "normalMap"), 4);
	glDrawElements(GL_TRIANGLES, entity.getModel().numIndices, GL_UNSIGNED_INT, (void*)(entity.getModel().offset * sizeof(GLuint)));
}

void renderTerrain(Terrain &terrain, GLuint shaderProgram, glm::mat4 &worldToView, glm::mat4 &perspective) {
	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, terrain.getTextureId2());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, terrain.getTextureId3());
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, terrain.getTextureId4());
	glUniform1i(glGetUniformLocation(shaderProgram, "tex2"), 1);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex3"), 2);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex4"), 3);
	renderEntity(terrain, shaderProgram, worldToView, perspective, true);
}

void renderSkybox(Entity &skybox, GLuint secondSkyboxTexture, float interpolation, GLuint shaderProgram, glm::mat4 &worldToView, glm::mat4 &perspective) {
	glDisable(GL_DEPTH_TEST);
	glUseProgram(shaderProgram);
	glm::mat4 translation = glm::translate(glm::mat4(), skybox.position);
	glm::mat4 scale = glm::scale(glm::mat4(), skybox.scale);
	glm::mat4 transformation = translation * scale;
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "worldToView"), 1, GL_FALSE, glm::value_ptr(worldToView));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(perspective));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelToWorld"), 1, GL_FALSE, glm::value_ptr(transformation));
	glBindVertexArray(skybox.getModel().vao);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureId);
	glUniform1i(glGetUniformLocation(shaderProgram, "cubeMap"), 10);
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_CUBE_MAP, secondSkyboxTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "cubeMap2"), 11);
	glUniform1f(glGetUniformLocation(shaderProgram, "interpolation"), interpolation);
	glDrawElements(GL_TRIANGLES, skybox.getModel().numIndices, GL_UNSIGNED_INT, (void*)0);
	glEnable(GL_DEPTH_TEST);
}

void renderParticleSystem(ParticleSystem &particleSystem, GLuint shaderProgram, int uniformLocationsTransformation[], int uniformLocationsProgress[], glm::mat4 &worldToView, glm::mat4 &perspective) {
	Particle *particle = particleSystem.particles;

	glUseProgram(shaderProgram);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "worldToView"), 1, GL_FALSE, glm::value_ptr(worldToView));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(perspective));

	glm::mat4 rotateBack;
	glm::mat4 identity = glm::mat4();
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "parentTransformation"), 1, GL_FALSE, glm::value_ptr(identity));
	rotateBack = identity;

	if (particle->getParentEntity()) {
		Entity parentEntity = *particle->getParentEntity();
		if (particleSystem.followParent) {
			glm::mat4 parentTransformation = getEntityTransformation(parentEntity);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "parentTransformation"), 1, GL_FALSE, glm::value_ptr(parentTransformation));
			glm::quat quaternion2 = directionToQuaternion(parentEntity.forward, parentEntity.up, DEFAULT_FORWARD, DEFAULT_UP);
			rotateBack = glm::inverse(glm::toMat4(quaternion2));
		}
		// Get rotation matrix for rotating back, from the rotation crated from parent transformation
	}

	// Extract camera rotation from worldToView
	glm::mat4 cameraRotation = glm::inverse(worldToView);
	cameraRotation[3][0] = 0;
	cameraRotation[3][1] = 0;
	cameraRotation[3][2] = 0;
	cameraRotation[3][3] = 1;
	
	glm::mat4 totalRotationGLM = rotateBack * cameraRotation;
	const float *totalRotation = glm::value_ptr(glm::transpose(totalRotationGLM));

	int numIndices = particleSystem.model.numIndices;
	int i = 0;

	glBindVertexArray(particleSystem.model.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, particleSystem.textureId);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDisable(GL_DEPTH_TEST);

	std::string uniformNameProgress = std::string("atlasSize");
	glUniform1i(glGetUniformLocation(shaderProgram, uniformNameProgress.c_str()), 8);

	LARGE_INTEGER startingTime, endingTime, elapsedMicroseconds;
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&startingTime);

	float transformation[] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	for (int j = 0; j < particleSystem.numParticles; i++, j++) {
		float rotation = particle->rotation;

		// Scale and first xy rotation
		float index0 = particle->scale.x * cos(rotation);
		float index1 = particle->scale.y * -sin(rotation);
		float index4 = particle->scale.x * sin(rotation);
		float index5 = particle->scale.y * cos(rotation);
		float index10 = particle->scale.z;

		// Rotation and scale
		transformation[0] = totalRotation[0] * index0 + totalRotation[1] * index4;
		transformation[1] = totalRotation[0] * index1 + totalRotation[1] * index5;
		transformation[2] = totalRotation[2] * index10;
		transformation[4] = totalRotation[4] * index0 + totalRotation[5] * index4;
		transformation[5] = totalRotation[4] * index1 + totalRotation[5] * index5;
		transformation[6] = totalRotation[6] * index10;
		transformation[8] = totalRotation[8] * index0 + totalRotation[9] * index4;
		transformation[9] = totalRotation[8] * index1 + totalRotation[9] * index5;
		transformation[10] = totalRotation[10] * index10;

		// Transform
		transformation[3] = particle->position.x;
		transformation[7] = particle->position.y;
		transformation[11] = particle->position.z;

		glUniformMatrix4fv(uniformLocationsTransformation[i], 1, GL_TRUE, &transformation[0]);
		glUniform1f(uniformLocationsProgress[i], particle->timeAlive / particle->lifetime);
		particle++;

		if (i >= NUM_PARTICLES_PER_DRAW_CALL - 1) {
			glDrawElementsInstanced(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0, i);
			i = 0;
		}
	}

	if (i > 0) {
		glDrawElementsInstanced(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0, i);
	}

	QueryPerformanceCounter(&endingTime);
	elapsedMicroseconds.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
	elapsedMicroseconds.QuadPart *= 1000000;
	elapsedMicroseconds.QuadPart /= frequency.QuadPart;
	//std::cout << elapsedMicroseconds.QuadPart << std::endl;
}

void runPhysics(Entity &entity, float dt) {
	glm::vec3 gravity = glm::vec3(0, -0.4f, 0);
	glm::vec3 netForce =  entity.impulse + gravity;
	entity.impulse = glm::vec3(0, 0, 0);
	entity.velocity = entity.velocity + netForce;
	entity.position = entity.position + entity.velocity * dt;
}

float getHeightAt(float *heightmap, int size, float tileSize, float positionX, float positionZ) {
	int tileX = positionX / tileSize;
	int tileZ = positionZ / tileSize;

	// Find the four corners of the grid
	glm::vec3 topLeftCorner = glm::vec3(tileX * tileSize, heightmap[tileX + tileZ * size], tileZ * tileSize);
	glm::vec3 bottomRightCorner = glm::vec3((tileX + 1) * tileSize, heightmap[tileX + (tileZ + 1) * size + 1], (tileZ + 1) * tileSize);
	glm::vec3 topRightCorner = glm::vec3((tileX + 1) * tileSize, heightmap[tileX + tileZ * size + 1], tileZ * tileSize);
	glm::vec3 bottomLeftCorner = glm::vec3(tileX  * tileSize, heightmap[tileX + (tileZ + 1) * size], (tileZ + 1) * tileSize);

	// Calculate tile penetration
	float tilePenetrationX = ((int)(positionX * 100) % (int)(tileSize * 100)) / (tileSize * 100);
	float tilePenetrationZ = ((int)(positionZ * 100) % (int)(tileSize * 100)) / (tileSize * 100);

	float calculatedY;
	// Check if top left triangle
	if (tilePenetrationX <= 1 - tilePenetrationZ) {
		glm::vec3 one = topLeftCorner;
		glm::vec3 two = topRightCorner;
		glm::vec3 three = bottomLeftCorner;
		GLfloat lambda1N = ((two.z - three.z)*(positionX - three.x) + (three.x - two.x)*(positionZ - three.z));
		GLfloat lambda1D = ((two.z - three.z)*(one.x - three.x) + (three.x - two.x)*(one.z - three.z));
		GLfloat lambda1 = lambda1N / lambda1D;
		GLfloat lambda2N = ((three.z - one.z)*(positionX - three.x) + (one.x - three.x)*(positionZ - three.z));
		GLfloat lambda2D = ((two.z - three.z)*(one.x - three.x) + (three.x - two.x)*(one.z - three.z));
		GLfloat lambda2 = lambda2N / lambda2D;
		GLfloat lambda3 = 1 - lambda1 - lambda2;
		calculatedY = one.y * lambda1 + two.y * lambda2 + three.y * lambda3;
	} else {
		glm::vec3 one = bottomRightCorner;
		glm::vec3 two = topRightCorner;
		glm::vec3 three = bottomLeftCorner;
		GLfloat lambda1N = ((two.z - three.z)*(positionX - three.x) + (three.x - two.x)*(positionZ - three.z));
		GLfloat lambda1D = ((two.z - three.z)*(one.x - three.x) + (three.x - two.x)*(one.z - three.z));
		GLfloat lambda1 = lambda1N / lambda1D;
		GLfloat lambda2N = ((three.z - one.z)*(positionX - three.x) + (one.x - three.x)*(positionZ - three.z));
		GLfloat lambda2D = ((two.z - three.z)*(one.x - three.x) + (three.x - two.x)*(one.z - three.z));
		GLfloat lambda2 = lambda2N / lambda2D;
		GLfloat lambda3 = 1 - lambda1 - lambda2;
		calculatedY = one.y * lambda1 + two.y * lambda2 + three.y * lambda3;
	}

	return calculatedY;
}

// Should also interpolate over triangle
void terrainCollision(float *heightmap, int size, float tileSize, Entity &entity) {
	float height = getHeightAt(heightmap, size, tileSize, entity.position.x, entity.position.z);
	if (height > entity.position.y + entity.centerToGroundContactPoint) {
		entity.position.y = height - entity.centerToGroundContactPoint;
		entity.velocity.y = 0;
	}
}

std::vector<unsigned char> loadPNG(std::string filename) {
	const char* filenameC = (const char*)filename.c_str();
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);
	std::vector<unsigned char> imageCopy(width * height * 4);
	for (unsigned i = 0; i < height; i++) {
		memcpy(&imageCopy[(height - i - 1) * width * 4], &image[i * width * 4], width * 4);
	}
	return image;
}

GLuint createSkybox(std::string x1, std::string x2, std::string y1, std::string y2, std::string z1, std::string z2) {
	GLuint texId;
	glGenTextures(1, &texId);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texId);

	std::vector<unsigned char> xPos = loadPNG(x1);
	std::vector<unsigned char> xNeg;
	std::vector<unsigned char> yPos;
	std::vector<unsigned char> yNeg;
	std::vector<unsigned char> zPos;
	std::vector<unsigned char> zNeg;
	const int size = 2048; // Should be returned from loader

	if (FAST_MODE) {
		xNeg = xPos;
		yPos = xPos;
		yNeg = xPos;
		zPos = xPos;
		zNeg = xPos;
	} else {
		xNeg = loadPNG(x2);
		yPos = loadPNG(y1);
		yNeg = loadPNG(y2);
		zPos = loadPNG(z1);
		zNeg = loadPNG(z2);
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &xPos[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &xNeg[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &yPos[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &yNeg[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &zPos[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &zNeg[0]);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return texId;
}

GLuint getShader(std::string vertexShaderPath, std::string fragmentShaderPath) {
	std::string commonSource = readFile("Source/common.glsl");
	const GLchar* commonSourceC = (const GLchar *)commonSource.c_str();

	std::string vertexSource = readFile(vertexShaderPath);
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar *vertexShaderFiles[2] = {
		commonSource.c_str(),
		vertexSource.c_str()
	};
	glShaderSource(vertexShader, 2, vertexShaderFiles, 0);

	glCompileShader(vertexShader);
	GLint isCompiledVertex = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiledVertex);
	if (isCompiledVertex == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> infoLog(maxLength);
		glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);

		std::string str(infoLog.begin(), infoLog.end());
		std::cout << str << std::endl;
	}

	std::string fragmentSource = readFile(fragmentShaderPath);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar *fragmentShaderFiles[2] = {
		commonSource.c_str(),
		fragmentSource.c_str()
	};
	glShaderSource(fragmentShader, 2, fragmentShaderFiles, 0);

	glCompileShader(fragmentShader);
	GLint isCompiledFragment = 0;
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiledFragment);
	if (isCompiledFragment == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> infoLog(maxLength);
		glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);

		std::string str(infoLog.begin(), infoLog.end());
		std::cout << str << std::endl;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	return program;
}

int steerMode = 0;
glm::vec3 cameraPosition = glm::vec3(10, 10, 10);
glm::vec3 cameraForward = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
Entity *entityToFollow = nullptr;
bool isForward, isBackward, isLeft, isUp, isRight, isDown, isStrideLeft, isStrideRight, jump, isShift;

void basicSteering(glm::vec3 &position, glm::vec3 &forward, glm::vec3 &up) {
	GLfloat rotationSpeed = 0.03f;
	GLfloat speed = 0.09f * (isShift ? 10 : 1);
	if (isForward)
	{
		position = position + forward * speed;
	}
	if (isBackward)
	{
		position = position - forward * speed;
	}
	if (isStrideLeft)
	{
		glm::vec3 left = glm::normalize(glm::cross(up, forward));
		position = position + left * speed;
	}
	if (isStrideRight)
	{
		glm::vec3 right = glm::normalize(glm::cross(forward, up));
		position = position + right * speed;
	}
	if (isLeft)
	{
		forward = glm::normalize(glm::rotate(forward, rotationSpeed, glm::vec3(0, 1, 0)));
		up = glm::normalize(glm::rotate(up, rotationSpeed, glm::vec3(0, 1, 0)));
	}
	if (isRight)
	{
		forward = glm::normalize(glm::rotate(forward, -rotationSpeed, glm::vec3(0, 1, 0)));
		up = glm::normalize(glm::rotate(up, -rotationSpeed, glm::vec3(0, 1, 0)));
	}
	if (isUp)
	{
		glm::vec3 right = glm::cross(forward, up);
		forward = glm::normalize(glm::rotate(forward, -rotationSpeed, right));
		up = glm::normalize(glm::rotate(up, -rotationSpeed, right));
	}
	if (isDown)
	{
		glm::vec3 right = glm::cross(forward, up);
		forward = glm::normalize(glm::rotate(forward, rotationSpeed, right));
		up = glm::normalize(glm::rotate(up, rotationSpeed, right));
	}
}

void printVector(glm::vec3 &v) {
	std::cout << v.x << ", " << v.y << ", " << v.z << std::endl;
}


void handleKeyChange(bool* currentValue, int action) {
	if (action == GLFW_PRESS) {
		*currentValue = true;
	} else if (action == GLFW_RELEASE) {
		*currentValue = false;
	}
}

void interpolateCamera(glm::vec3 &targetPosition, glm::vec3 &cameraPosition, float dt) {
	glm::vec3 direction = glm::normalize(targetPosition - cameraPosition);
	if (glm::length(direction) > 0.5f) {
		cameraPosition = cameraPosition + direction * dt * glm::length(targetPosition - cameraPosition) / 1.0f * 16.0f;
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		steerMode++;
		if (steerMode == 3) {
			steerMode = 0;
		}
	}


	if (key == GLFW_KEY_LEFT_SHIFT) {
		handleKeyChange(&isShift, action);
	}
	
	if (key == GLFW_KEY_W) {
		handleKeyChange(&isForward, action);
	}

	if (key == GLFW_KEY_S) {
		handleKeyChange(&isBackward, action);
	}

	if (key == GLFW_KEY_A) {
		handleKeyChange(&isStrideLeft, action);
	}

	if (key == GLFW_KEY_D) {
		handleKeyChange(&isStrideRight, action);
	}

	if (key == GLFW_KEY_LEFT) {
		handleKeyChange(&isLeft, action);
	}

	if (key == GLFW_KEY_RIGHT) {
		handleKeyChange(&isRight, action);
	}

	if (key == GLFW_KEY_UP) {
		handleKeyChange(&isUp, action);
	}

	if (key == GLFW_KEY_DOWN) {
		handleKeyChange(&isDown, action);
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		jump = true;
	}
}

static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}


void makeRunwayOnHeightmap(float *heightmap, int size) {
	float avgHeight = heightmap[3 * size + 8] + heightmap[80 * size + 8] / 2.0f;
	for (int x = 3; x <= 8; x++) {
		for (int z = 3; z <= 30; z++) {
			heightmap[z * size + x] = avgHeight;
		}
	}
}

void log(std::string output) {
	std::cout << output << std::endl;
}

void glDebugMessageCallbackFunction(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
	if (type == GL_DEBUG_TYPE_MARKER) {
		return;
	}

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		log("Source: GL_DEBUG_SOURCE_API");
		break;

	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		log("Source: GL_DEBUG_SOURCE_WINDOW_SYSTEM");
		break;

	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		log("Source: GL_DEBUG_SOURCE_SHADER_COMPILER");
		break;

	case GL_DEBUG_SOURCE_THIRD_PARTY:
		log("Source: GL_DEBUG_SOURCE_THIRD_PARTY");
		break;

	case GL_DEBUG_SOURCE_APPLICATION:
		log("Source: GL_DEBUG_SOURCE_APPLICATION");
		break;

	case GL_DEBUG_SOURCE_OTHER:
		log("Source: GL_DEBUG_SOURCE_OTHER");
		break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		log("Type: GL_DEBUG_TYPE_ERROR");
		break;

	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		log("Type: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR");
		break;

	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		log("Type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR");
		break;

	case GL_DEBUG_TYPE_PORTABILITY:
		log("Type: GL_DEBUG_TYPE_PORTABILITY");
		break;

	case GL_DEBUG_TYPE_PERFORMANCE:
		log("Type: GL_DEBUG_TYPE_PERFORMANCE");
		break;

	case GL_DEBUG_TYPE_MARKER:
		log("Type: GL_DEBUG_TYPE_MARKER");
		break;

	case GL_DEBUG_TYPE_PUSH_GROUP:
		log("Type: GL_DEBUG_TYPE_PUSH_GROUP");
		break;

	case GL_DEBUG_TYPE_POP_GROUP:
		log("Type: GL_DEBUG_TYPE_POP_GROUP");
		break;

	case GL_DEBUG_TYPE_OTHER:
		log("Type: GL_DEBUG_TYPE_OTHER");
		break;
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		log("Severity: GL_DEBUG_SEVERITY_HIGH");
		break;

	case GL_DEBUG_SEVERITY_MEDIUM:
		log("Severity: GL_DEBUG_SEVERITY_MEDIUM");
		break;

	case GL_DEBUG_SEVERITY_LOW:
		log("Severity: GL_DEBUG_SEVERITY_LOW");
		break;

	case GL_DEBUG_SEVERITY_NOTIFICATION:
		log("Severity: GL_DEBUG_SEVERITY_NOTIFICATION");
		break;
	}

	log(id);
	log(message);
}

int program();

int main() {
	return program();
}

glm::mat4 getEntityTransformation(Entity &entity, bool useRotation) {
	glm::mat4 toPivot = glm::translate(glm::mat4(), -entity.getRotationPivot());
	glm::mat4 translation = glm::translate(glm::mat4(), entity.position);
	glm::mat4 scale = glm::scale(glm::mat4(), entity.scale);
	glm::quat quaternion = directionToQuaternion(entity.forward, entity.up, DEFAULT_FORWARD, DEFAULT_UP);
	glm::mat4 rotation = glm::toMat4(quaternion);
	glm::mat4 transformation = translation * glm::inverse(toPivot) * rotation * toPivot * scale;
	if (entity.getParentEntity()) {
		return getEntityTransformation(*entity.getParentEntity()) * transformation;
	}
	else {
		return transformation;
	}
}

int program() {
	const int windowHeight = 900;
	const int windowWidth = 1700;
	glm::mat4 perspective = glm::perspective<GLfloat>(0.8f, windowWidth / (float)windowHeight, .1f, 1500);
	glm::mat4 perspectiveForSun = glm::perspective<GLfloat>(0.8f, windowWidth / (float)windowHeight, .1f, 51500);

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Flight Simulator", NULL, NULL);
	if (window == NULL) {
		std::cerr << "Error! Failed to create Window!" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Error! Failed to initlize OpenGL" << std::endl;
	}

	GLuint ids[] = { 0x20071 };
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 1, ids, GL_FALSE);
	glDebugMessageCallback(glDebugMessageCallbackFunction, 0);

	GLuint modelShader = getShader("Source/modelVS.glsl", "Source/modelFS.glsl");
	GLuint skyboxShader = getShader("Source/skyboxVS.glsl", "Source/skyboxFS.glsl");
	GLuint terrainShader = getShader("Source/terrainVS.glsl", "Source/terrainFS.glsl");
	GLuint instancedShader = getShader("Source/instancedVS.glsl", "Source/instancedFS.glsl");
	int uniformLocationInstanceShaderTransformation[NUM_PARTICLES_PER_DRAW_CALL];
	int uniformLocationInstanceShaderProgress[NUM_PARTICLES_PER_DRAW_CALL];
	for (int i = 0; i < NUM_PARTICLES_PER_DRAW_CALL; i++) {
		std::string uniformName = std::string("modelToWorld[") + std::to_string(i) + std::string("]");
		std::string uniformNameColor = std::string("progress[") + std::to_string(i) + std::string("]");
		uniformLocationInstanceShaderTransformation[i] = glGetUniformLocation(instancedShader, uniformName.c_str());
		uniformLocationInstanceShaderProgress[i] = glGetUniformLocation(instancedShader, uniformNameColor.c_str());
	}

	int size = 2049;
	int tileSizeXZ = 2;
	int tileSizeY = 1;
	float smothness = 0.3f;
	if (FAST_MODE) {
		size = 129;
		tileSizeXZ = 10;
		smothness = 1;
	}

	time_t seed = 1519128009; // Splat map is built after this seed, so don't change it
	float *heightmapData = new float[size * size];
	diamondSquare(heightmapData, size, smothness, seed);
	makeRunwayOnHeightmap(heightmapData, size);

	Model terrain = heightmapToModel(heightmapData, size, size, tileSizeXZ, tileSizeY, tileSizeXZ, 40 * tileSizeXZ);

	Terrain ground = Terrain();
	ground.setModel(terrain);
	ground.position = glm::vec3(0, 0, 0);
	ground.scale = glm::vec3(1, 1, 1);
	ground.textureId = loadPNGTexture("Resources/grass512.png");
	ground.setTextureId2(loadPNGTexture("Resources/sand512.png"));
	ground.setTextureId3(loadPNGTexture("Resources/rock512.png"));
	ground.setTextureId4(loadPNGTexture("Resources/terrain-splatmap.png"));

	GLuint jasTexture = loadPNGTexture("Resources/jas.png");
	GLuint jasNormalMap = loadPNGTexture("Resources/normalmap.png");
	std::vector<Entity*> airplane = loadJAS39Gripen("Resources/jas.obj");
	airplane[0]->position = glm::vec3(10, 10, 20);
	airplane[0]->scale = glm::vec3(0.2f, 0.2f, 0.2f);
	airplane[0]->centerToGroundContactPoint = -0.2f;
	for (std::vector<Entity*>::iterator iter = airplane.begin(); iter != airplane.end(); iter++) {
		(*iter)->textureId = jasTexture;
		(*iter)->normalMapId = jasNormalMap;
	}

	Entity skybox = Entity();
	skybox.setModel(getVAOCube());
	skybox.scale = glm::vec3(20, 20, 20);
	skybox.position = glm::vec3(0, -10, 0);
	GLuint secondSkybox;
	if (FAST_MODE) {
		skybox.textureId = createSkybox("Resources/skybox-x-.png", "Resources/skybox-x+.png", "Resources/skybox-y+.png", "Resources/skybox-y-.png", "Resources/skybox-z-.png", "Resources/skybox-z+.png");
		secondSkybox = skybox.textureId;
	} else {
		skybox.textureId = createSkybox("Resources/skybox-x-.png", "Resources/skybox-x+.png", "Resources/skybox-y+.png", "Resources/skybox-y-.png", "Resources/skybox-z-.png", "Resources/skybox-z+.png");
		secondSkybox = createSkybox("Resources/skybox-night-x-.png", "Resources/skybox-night-x+.png", "Resources/skybox-night-y+.png", "Resources/skybox-night-y-.png", "Resources/skybox-night-z-.png", "Resources/skybox-night-z+.png");
	}

	Entity cube = Entity();
	cube.setModel(getVAOCube());
	cube.scale = glm::vec3(0.1, 0.1, 0.1);
	cube.position = glm::vec3(0, 0, 0);
	cube.textureId = loadPNGTexture("Resources/grass512.png");
	cube.normalMapId = loadPNGTexture("Resources/normalmap.png");

	Entity player = Entity();
	player.setModel(getVAOCube());
	player.scale = glm::vec3(0.1, 0.4, 0.1);
	player.position = glm::vec3(10, 100, 10);
	player.getModel().color = glm::vec4(0.7f, 0.84f, 0.54f, 1.0f);
	player.centerToGroundContactPoint = -0.4f;

	std::vector<Light*> lights;

	Light playerLight = createPointLight(glm::vec3(0, 1.2, 0), glm::vec3(0.05, 0.05, 0.05), 3, 0.1, 0.1, glm::vec3(0.3, 1, 0.8));
	playerLight.setParentEntity(&player);
	lights.push_back(&playerLight);

	Light sun = createDirectionalLight(glm::vec3(50, 50, 50), glm::vec3(100, 100, 100), 3.4, glm::vec3(0, -1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 1, 1));
	lights.push_back(&sun);

	Light airplaneSpotlight = createSpotlight(glm::vec3(0, 0, 10), glm::vec3(0.1, 0.1, 0.1), 10, 0.00001, 0.1, 0.2f, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec3(1, 1, 1));
	airplaneSpotlight.setParentEntity(airplane[0]);
	lights.push_back(&airplaneSpotlight);

	Light airplaneWingLight = createPointLight(glm::vec3(-4, 0.1, -3), glm::vec3(0.05, 0.05, 0.05), 3, 0.1, 0.1, glm::vec3(1, 0.2, 0.2));
	airplaneWingLight.setParentEntity(airplane[0]);
	lights.push_back(&airplaneWingLight);

	Light worldGreenMovingLight = createPointLight(glm::vec3(100, 50, 100), glm::vec3(1, 1, 1), 10, 0.001, 0.001, glm::vec3(0, 1, 0.55));
	worldGreenMovingLight.centerToGroundContactPoint = -10;
	lights.push_back(&worldGreenMovingLight);

	Light fireLight = createPointLight(glm::vec3(17, 80, 43), glm::vec3(0.1, 0.1, 0.1), 20, 0.1, 0.1, glm::vec3(1, 1, 1));
	lights.push_back(&fireLight);

	// Particles
	Model particleModel = getVAOQuad();
	ParticleSystem smoke = ParticleSystem(300 * 2);
	smoke.model = particleModel;
	smoke.particlesPerSecond = 300;
	smoke.timeSinceLastSpawn = 0;
	smoke.minLifetime = 0.9f;
	smoke.maxLifetime = 2.0f;
	smoke.minSize = 0.2f;
	smoke.maxSize = 0.4f;
	smoke.sphereRadiusSpawn = 1.0f;
	smoke.textureId = loadPNGTexture("Resources/particle-atlas2.png");
	smoke.atlasSize = 8;
	smoke.velocity = 3.25f;
	smoke.position = glm::vec3(0, 0.25f, -5.9f);
	smoke.setDirection(-0.08, 0.08, -0.08, 0.08, -1.1, -1);
	smoke.parentEntity = airplane[0];
	smoke.followParent = true;

	// Wingtip
	ParticleSystem wingtip = ParticleSystem(20000);
	wingtip.model = particleModel;
	wingtip.particlesPerSecond = 2000;
	wingtip.timeSinceLastSpawn = 0;
	wingtip.minLifetime = 0.1f;
	wingtip.maxLifetime = 2.4f;
	wingtip.minSize = .02f;
	wingtip.maxSize = .04f;
	wingtip.sphereRadiusSpawn = 0.2f;
	wingtip.textureId = loadPNGTexture("Resources/particle-atlas3.png");
	wingtip.atlasSize = 8;
	wingtip.velocity = 0.0000001f;
	wingtip.position = glm::vec3(4.0f, 0.0f, -4.2f);
	wingtip.setDirection(-0.05f, 0.05f, -0.05f, 0.05f, -400, -300);
	wingtip.parentEntity = airplane[0];
	wingtip.followParent = false;

	glClearColor(1, 0.43, 0.66, 0.0f);
	glEnable(GL_DEPTH_TEST);

	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);
	//glCullFace(GL_BACK);

	bool boom = false;
	float lastTime = glfwGetTime();
	float currentTime;
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		float currentTime = glfwGetTime();
		float dt = currentTime - lastTime;
		lastTime = currentTime;
		std::string title = std::string("Frame time: ") + std::to_string(dt) + std::string(" micro seconds");
		glfwSetWindowTitle(window, title.c_str());

		if (steerMode == 0) { // Camera
			basicSteering(cameraPosition, cameraForward, cameraUp);
			entityToFollow = nullptr;
		} else if (steerMode == 1) { // Airplane
			if (!boom) {
				steerAirplane(*airplane[0], *airplane[18], *airplane[1], *airplane[2], *airplane[14], isForward ? 1 : (isBackward ? -1 : 0), isLeft ? 1 : (isRight ? -1 : 0), isDown ? 1 : (isUp ? -1 : 0), dt);
				entityToFollow = airplane[0];
			}
		} else if (steerMode == 2) { // Player
			entityToFollow = &player;
			basicSteering(player.position, player.forward, player.up);
		}

		bool collision = getHeightAt(heightmapData, size, tileSizeXZ, airplane[0]->position.x, airplane[0]->position.z) >= airplane[0]->position.y - 0.2f;

		if (!boom && collision && glm::length(airplane[0]->velocity) > 120) {
			std::cout << "BOOOOM!" << std::endl;
			glm::vec3 velocity = airplane[0]->velocity;
			for (std::vector<Entity*>::iterator iter = ++airplane.begin(); iter != airplane.end(); iter++) {
				(*iter)->setParentEntity(nullptr);
				(*iter)->position = airplane[0]->position;
				(*iter)->scale = airplane[0]->scale;
				(*iter)->forward = airplane[0]->forward;
				(*iter)->up = airplane[0]->up;
				(*iter)->centerToGroundContactPoint = airplane[0]->centerToGroundContactPoint;
				(*iter)->velocity.x = (random(0, abs(velocity.x) * 100)) / 50.0f;
				(*iter)->velocity.y = (random(0, abs(velocity.y) * 100)) / 50.0f;
				(*iter)->velocity.z = (random(0, abs(velocity.z) * 100)) / 50.0f;
				if (velocity.x < 0) {
					(*iter)->velocity.x = -(*iter)->velocity.x;
				}
				if (velocity.y < 0) {
					(*iter)->velocity.y = -(*iter)->velocity.y;
				}
				if (velocity.z < 0) {
					(*iter)->velocity.z = -(*iter)->velocity.z;
				}
			}
			//steerMode = 0;
			boom = true;
		}

		if (boom) {
			if (steerMode == 1) {
				entityToFollow = airplane[0];
			}
			for (std::vector<Entity*>::iterator iter = ++airplane.begin(); iter != airplane.end(); iter++) {
				//airplanePhysics(**iter, *airplane[18], *airplane[2], dt);
				normalPhysics(**iter, dt);
				terrainCollision(heightmapData, size, tileSizeXZ, **iter);
			}
		} else {
			airplanePhysics(*airplane[0], *airplane[18], *airplane[2], dt);
		}
		
		terrainCollision(heightmapData, size, tileSizeXZ, *airplane[0]);
		runPhysics(player, dt);
		terrainCollision(heightmapData, size, tileSizeXZ, player);

		for (int i = 0; i < lights.size(); i++) {
			if (lights[i]->getParentEntity()) {
				continue;
			}
			runPhysics(*lights[i], dt);
			terrainCollision(heightmapData, size, tileSizeXZ, *lights[i]);
		}

		// Camera
		glm::mat4 cam = glm::lookAt(cameraPosition, cameraPosition + cameraForward * 14.0f, cameraUp);
		if (entityToFollow) {
			glm::vec3 targetPosition = entityToFollow->position + -entityToFollow->forward * 22.5f * (boom ? 2.0f : 1.0f) + entityToFollow->up * 1.0f  * (boom ? 2.0f : 1.0f);
			//cameraPosition = targetPosition;

			interpolateCamera(targetPosition, cameraPosition, dt);
			float terrainHeightAtCamera = getHeightAt(heightmapData, size, tileSizeXZ, cameraPosition.x, cameraPosition.z);
			if (cameraPosition.y <= terrainHeightAtCamera + 0.05f) {
				cameraPosition.y = terrainHeightAtCamera + 0.05f;
			}
			cameraForward = glm::normalize(entityToFollow->position - cameraPosition);
			cam = glm::lookAt(cameraPosition, entityToFollow->position, entityToFollow->up);
			cameraUp = entityToFollow->up;
		}

		worldGreenMovingLight.position.x = 100 * sin(glfwGetTime() / (float)10) + 400;
		worldGreenMovingLight.position.z = 100 * cos(glfwGetTime() / (float)10) + 400;

		airplaneWingLight.intensity = 4;
		worldGreenMovingLight.intensity = 4 * sin(glfwGetTime() * 2) + 4;
		if ((int)(glfwGetTime() * 10) % 10 != 0) {
			airplaneWingLight.intensity = 0;
		}

		// Animate sun (make sure direction always faces middle of ground
		float centerPosition = (float)(size * tileSizeXZ) / 2.0f;
		glm::vec3 center = glm::vec3(centerPosition, getHeightAt(heightmapData, size, tileSizeXZ, centerPosition, centerPosition), centerPosition);
		float time = glfwGetTime() / 1;// 5000;
		sun.position.z = centerPosition;
		sun.position.y = cos(time) * centerPosition * 2 + center.y;
		sun.position.x = sin(time) * centerPosition * 2 + centerPosition;
		sun.up = glm::vec3(0, 0, 1);
		sun.forward = glm::normalize(center - sun.position);
		float interpolation = (cos(time) + 1) / 2.0f;

		// Animate fire light
		if ((int)(glfwGetTime() * 100) % 4 == 3) {
			fireLight.intensity = fireLight.intensity - (random(0, 8) - 4);
		}

		// Render entities
		renderSkybox(skybox, secondSkybox, interpolation, skyboxShader, cam, perspective);
		bindLight(terrainShader, lights);
		bindLight(modelShader, lights);
		renderTerrain(ground, terrainShader, cam, perspective);
		for (int i = 0; i < airplane.size(); i++) {
			if (i == 8 || i == 0) {
				continue;
			}
			renderEntity(*airplane[i], modelShader, cam, perspective, true);
		}
		renderEntity(player, modelShader, cam, perspective, true);
		renderEntity(*airplane[0], modelShader, cam, perspective, true);
		renderEntity(*airplane[8], modelShader, cam, perspective, true);
		renderEntity(cube, modelShader, cam, perspective, true);

		// Particles
		Entity parentEntity = *airplane[0];
		updateParticleSystem(smoke, cameraPosition, cameraForward, dt);
		updateParticleSystem(wingtip, cameraPosition, cameraForward, dt);
		if (smoke.numParticles > 0) {
			renderParticleSystem(smoke, instancedShader, uniformLocationInstanceShaderTransformation, uniformLocationInstanceShaderProgress, cam, perspective);
		}
		if (wingtip.numParticles > 0) {
			renderParticleSystem(wingtip, instancedShader, uniformLocationInstanceShaderTransformation, uniformLocationInstanceShaderProgress, cam, perspective);
		}

		// Draw lights
		for (int i = 0; i < lights.size(); i++) {
			renderEntity(*lights[i], modelShader, cam, perspectiveForSun, false);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete heightmapData;
	
	for (std::vector<Entity*>::iterator iter = airplane.begin(); iter != airplane.end(); iter++) {
		delete *iter;
	}

	return 0;
}