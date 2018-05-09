#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtx/euler_angles.hpp> 
#include <glm/gtx/transform.hpp> 
#include <glm/gtx/rotate_vector.hpp> 
#include <glm/gtx/quaternion.hpp>

#include "Rendering.h"

void bindLight(GLuint shaderProgram, std::vector<Light*> lights) {
	glUseProgram(shaderProgram);
	for (int i = 0; i < lights.size(); i++) {
		Light *light = lights[i];
		glm::mat4 transformation = getEntityTransformation(*light);
		glm::vec4 worldPosition = transformation * glm::vec4(0, 0, 0, 1);
		glm::vec4 worldForward = glm::normalize(transformation * glm::vec4(0, 0, 1, 0));
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
	glUniform1i(glGetUniformLocation(shaderProgram, uniformNameProgress.c_str()), particleSystem.atlasSize);

	//LARGE_INTEGER startingTime, endingTime, elapsedMicroseconds;
	//LARGE_INTEGER frequency;
	//QueryPerformanceFrequency(&frequency);
	//QueryPerformanceCounter(&startingTime);

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

	//QueryPerformanceCounter(&endingTime);
	//elapsedMicroseconds.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
	//elapsedMicroseconds.QuadPart *= 1000000;
	//elapsedMicroseconds.QuadPart /= frequency.QuadPart;
	//std::cout << elapsedMicroseconds.QuadPart << std::endl;
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
	}
	else {
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