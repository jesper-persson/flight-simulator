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

#include <lodepng.h>
#include "Common.h"
#include "DiamondSquare.h"
#include "Physics.h"

glm::quat directionToQuaternion(glm::vec3 forward, glm::vec3 up, glm::vec3 defaultForward, glm::vec3 defaultUp) {
	assert(std::abs(glm::dot(forward, up)) <= 0.00001f, "Forward and up must be perpendicular");

	glm::vec3 rotationAxis = glm::normalize(glm::cross(defaultForward, forward));
	if (glm::dot(defaultForward, forward) > 0.99999f) {
		rotationAxis = forward;
	} else if (glm::dot(defaultForward, forward) < -0.99999f) {
		rotationAxis = up;
	}
	float angle = acos(glm::dot(defaultForward, forward));
	glm::highp_quat quatForward = glm::rotate(glm::quat(), angle, rotationAxis);

	// Account for roll using up vector
	glm::vec3 newUp = glm::normalize(quatForward * defaultUp);
	float upAngle = acos(glm::dot(up, newUp));
	if (glm::dot(newUp, up) > 0.99999f) {
		return quatForward;
	}
	glm::highp_quat quatUp = glm::rotate(glm::quat(), upAngle, glm::normalize(glm::cross(newUp, up)));

	return  quatUp * quatForward;
}

glm::mat4 getEntityTransformation(Entity &entity) {
	glm::mat4 toPivot = glm::translate(glm::mat4(), -entity.getRotationPivot());
	glm::mat4 translation = glm::translate(glm::mat4(), entity.position);
	glm::mat4 scale = glm::scale(glm::mat4(), entity.scale);
	glm::quat quaternion = directionToQuaternion(entity.forward, entity.up, DEFAULT_FORWARD, DEFAULT_UP);
	glm::mat4 rotation = glm::toMat4(quaternion);
	glm::mat4 transformation = translation * glm::inverse(toPivot) * rotation * toPivot * scale;
	if (entity.getParentEntity()) {
		return getEntityTransformation(*entity.getParentEntity()) * transformation;
	} else {
		return transformation;
	}
}

void bindLight(GLuint shaderProgram, Light *light, int numLights) {
	glUseProgram(shaderProgram);
	for (int i = 0; i < numLights; i++) {
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
	float height = heightmap[tileZ * size + tileX];
	return height;
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
	std::vector<unsigned char> xNeg = loadPNG(x2);
	std::vector<unsigned char> yPos = loadPNG(y1);
	std::vector<unsigned char> yNeg = loadPNG(y2);
	std::vector<unsigned char> zPos = loadPNG(z1);
	std::vector<unsigned char> zNeg = loadPNG(z2);
	const int size = 2048; // Should be returned from loader

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

glm::vec3 cameraPosition = glm::vec3(10, 10, 10);
glm::vec3 cameraForward = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
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

void interpolateCamera(glm::vec3 &targetPosition, glm::vec3 &cameraPosition) {
	float dt = 0.01f;
	glm::vec3 direction = glm::normalize(targetPosition - cameraPosition);
	cameraPosition = cameraPosition + direction * dt * glm::length(targetPosition - cameraPosition) / 1.0f * 18.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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

int main() {
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

	const int size = 257;
	const int tileSizeXZ = 10;
	const int tileSizeY = 1;
	time_t seed = 1519128009; // Splat map is built after this seed, so don't change it
	float *heightmapData = new float[size * size];
	diamondSquare(heightmapData, size, 1.3f, seed);
	makeRunwayOnHeightmap(heightmapData, size);
	const int numSubdivisions = 2;

	Entity *terrainEntities[numSubdivisions * numSubdivisions];
	for (int x = 0; x < numSubdivisions; x++) {
		for (int z = 0; z < numSubdivisions; z++) {
		}
	}

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
	GLuint jasNormalMap = loadPNGTexture("Resources/metal-normalmap.png");
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
	skybox.textureId = createSkybox("Resources/skybox-x-.png", "Resources/skybox-x+.png", "Resources/skybox-y+.png", "Resources/skybox-y-.png", "Resources/skybox-z-.png", "Resources/skybox-z+.png");
	GLuint secondSkybox = createSkybox("Resources/skybox-night-x-.png", "Resources/skybox-night-x+.png", "Resources/skybox-night-y+.png", "Resources/skybox-night-y-.png", "Resources/skybox-night-z-.png", "Resources/skybox-night-z+.png");
	

	Entity cube = Entity();
	cube.setModel(getVAOCube());
	cube.scale = glm::vec3(1, 1, 1);
	cube.position = glm::vec3(10, 100, 14);
	cube.textureId = loadPNGTexture("Resources/grass512.png");
	cube.normalMapId = loadPNGTexture("Resources/normalmap.png");

	Light directionalLight = Light();
	directionalLight.lightType = LightType::DIRECTIONAL_LIGHT;
	directionalLight.forward = glm::normalize(glm::vec3(0, -1, 0));
	directionalLight.up = glm::normalize(glm::vec3(0, 0, 1));
	directionalLight.color = glm::vec3(1, 1, 1);
	directionalLight.intensity = 3.4;
	Model sunModel = getVAOCube();
	sunModel.color = glm::vec4(directionalLight.color.x, directionalLight.color.y, directionalLight.color.z, 1);
	directionalLight.setModel(sunModel);
	directionalLight.position = glm::vec3(50, 50, 50);
	directionalLight.scale = glm::vec3(100, 100, 100);

	Light spotlight = Light();
	spotlight.setParentEntity(airplane[0]);
	spotlight.lightType = LightType::SPOTLIGHT;
	spotlight.cutoffAngle = 0.2f;
	spotlight.attenuationC1 = 0.00001;
	spotlight.attenuationC2 = 0;
	spotlight.forward = glm::normalize(glm::vec3(0, -1, 4));
	spotlight.up = glm::normalize(glm::vec3(0, 4, 1));
	spotlight.color = glm::vec3(0.9, 0.94, 1);
	spotlight.intensity = 10;
	Model spotlightModel = getVAOCube();
	spotlightModel.color = glm::vec4(spotlight.color.x, spotlight.color.y, spotlight.color.z, 1);
	spotlight.setModel(spotlightModel);
	spotlight.position = glm::vec3(0, 0, 10);
	spotlight.scale = glm::vec3(0.1, 0.1, 0.1);

	Light pointLight = Light();
	pointLight.setParentEntity(airplane[0]);
	pointLight.lightType = LightType::POINT_LIGHT;
	pointLight.attenuationC1 = 0.1;
	pointLight.attenuationC2 = 0.1;
	pointLight.color = glm::vec3(1 , 0.5 , 0.5);
	pointLight.intensity = 3;
	Model pointLightModel = getVAOCube();
	pointLightModel.color = glm::vec4(pointLight.color.x, pointLight.color.y, pointLight.color.z, 1);
	pointLight.setModel(pointLightModel);
	pointLight.position = glm::vec3(-4, 0.1, -3);
	pointLight.scale = glm::vec3(0.05, 0.05, 0.05);

	Light pointLight2 = Light();
	pointLight2.lightType = LightType::POINT_LIGHT;
	pointLight2.attenuationC1 = 0.001;
	pointLight2.attenuationC2 = 0.001;
	pointLight2.centerToGroundContactPoint = -10;
	pointLight2.color = glm::vec3(0, 1, 0.5);
	pointLight2.intensity = 10;
	Model pointLightModel2 = getVAOCube();
	pointLightModel2.color = glm::vec4(pointLight2.color.x, pointLight2.color.y, pointLight2.color.z, 1);
	pointLight2.setModel(pointLightModel2);
	pointLight2.position = glm::vec3(100, 50, 100);
	pointLight2.scale = glm::vec3(1.05, 1.05, 1.05);

	Light lights[10];
	lights[0] = directionalLight;
	lights[1] = spotlight;
	lights[2] = pointLight;
	lights[3] = pointLight2;
	int numLights = 4;

	glClearColor(1, 0.43, 0.66, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);
	//glCullFace(GL_BACK);

	float lastTime = glfwGetTime();
	float currentTime;
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		float currentTime = glfwGetTime();
		float dt = currentTime - lastTime;
		lastTime = currentTime;
		//basicSteering(cameraPosition, cameraForward, cameraUp);

		steerAirplane(*airplane[0], *airplane[18], *airplane[1], *airplane[2], *airplane[14], isForward ? 1 : (isBackward ? -1 : 0), isLeft ? 1 : (isRight ? -1 : 0), isDown ? 1 : (isUp ? -1 : 0), dt);
		airplanePhysics(*airplane[0], *airplane[18], *airplane[2], isForward ? 1 : (isBackward ? -1 : 0), isLeft ? 1 : (isRight ? -1 : 0), isDown ? 1 : (isUp ? -1 : 0), dt);
		terrainCollision(heightmapData, size, tileSizeXZ, *airplane[0]);

		for (int i = 0; i < numLights; i++) {
			if (lights[i].getParentEntity()) {
				continue;
			}
			runPhysics(lights[i], dt);
			terrainCollision(heightmapData, size, tileSizeXZ, lights[i]);
		}

		// Normal camera
		glm::mat4 cam = glm::lookAt(cameraPosition, cameraPosition + cameraForward * 10.0f, cameraUp);

		glm::vec3 targetPosition = airplane[0]->position + -airplane[0]->forward * 2.5f;
		interpolateCamera(targetPosition, cameraPosition);
		cam = glm::lookAt(cameraPosition, airplane[0]->position, airplane[0]->up);

		lights[3].position.x = 100 * sin(glfwGetTime() / (float)10) + 400;
		lights[3].position.z = 100 * cos(glfwGetTime() / (float)10) + 400;

		lights[2].intensity = 4;
		lights[3].intensity = 4 * sin(glfwGetTime() * 2) + 4;
		if ((int)(glfwGetTime()*10) % 10 != 0) {
			lights[2].intensity = 0;
		}

		// Animate sun (make sure direction always faces middle of ground
		Light *sun = &lights[0];
		float centerPosition = (float)(size * tileSizeXZ) / 2.0f;
		glm::vec3 center = glm::vec3(centerPosition, getHeightAt(heightmapData, size, tileSizeXZ, centerPosition, centerPosition), centerPosition);
		float time = glfwGetTime() / 15;
		sun->position.z = centerPosition;
		sun->position.y = cos(time) * centerPosition * 2 + center.y;
		sun->position.x = sin(time) * centerPosition * 2 + centerPosition;
		sun->up = glm::vec3(0, 0, 1);
		sun->forward = glm::normalize(center - sun->position);
		float interpolation = (cos(time) + 1) / 2.0f;

		// Render entities
		renderSkybox(skybox, secondSkybox, interpolation, skyboxShader, cam, perspective);
		bindLight(terrainShader, lights, numLights);
		bindLight(modelShader, lights, numLights);
		renderTerrain(ground, terrainShader, cam, perspective);
		for (int i = 0; i < airplane.size(); i++) {
			if (i == 8 || i == 0) {
				continue;
			}
			renderEntity(*airplane[i], modelShader, cam, perspective, true);
		}
		renderEntity(*airplane[0], modelShader, cam, perspective, true);
		renderEntity(*airplane[8], modelShader, cam, perspective, true);
		renderEntity(cube, modelShader, cam, perspective, true);

		// Draw lights
		for (int i = 0; i < numLights; i++) {
			renderEntity(lights[i], modelShader, cam, perspectiveForSun, false);
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