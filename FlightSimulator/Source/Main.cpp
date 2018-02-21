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

void renderEntity(Entity &entity, GLuint shaderProgram, glm::mat4 &worldToView, glm::mat4 &perspective) {
	glm::mat4 translation = glm::translate(glm::mat4(), entity.position);
	glm::mat4 scale = glm::scale(glm::mat4(), entity.scale);
	glm::quat quaternion = directionToQuaternion(entity.forward, entity.up, DEFAULT_FORWARD, DEFAULT_UP);
	glm::mat4 rotation = glm::toMat4(quaternion);

	glm::mat4 transformation = translation * rotation * scale;
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "worldToView"), 1, GL_FALSE, glm::value_ptr(worldToView));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(perspective));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelToWorld"), 1, GL_FALSE, glm::value_ptr(transformation));
	glBindVertexArray(entity.vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, entity.textureId);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);

	glDrawElements(GL_TRIANGLES, entity.numIndices, GL_UNSIGNED_INT, (void*)0);
}

void renderTerrain(Terrain &terrain, GLuint shaderProgram, glm::mat4 &worldToView, glm::mat4 &perspective) {
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, terrain.getTextureId2());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, terrain.getTextureId3());
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, terrain.getTextureId4());
	glUniform1i(glGetUniformLocation(shaderProgram, "tex2"), 1);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex3"), 2);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex4"), 3);
	glUniform1i(glGetUniformLocation(shaderProgram, "isTerrain"), 1);
	renderEntity(terrain, shaderProgram, worldToView, perspective);
	glUniform1i(glGetUniformLocation(shaderProgram, "isTerrain"), 0);
}

void runPhysics(Entity &ground, Entity &entity, float dt) {
	glm::vec3 gravity = glm::vec3(0, -0.4f, 0);
	glm::vec3 netForce =  entity.impulse + gravity;
	entity.impulse = glm::vec3(0, 0, 0);
	entity.velocity = entity.velocity + netForce;
	entity.position = entity.position + entity.velocity * dt;
}

// Should also interpolate over triangle
void terrainCollision(float *heightmap, int size, float tileSize, Entity &entity) {
	int tileX = entity.position.x / tileSize;
	int tileZ = entity.position.z / tileSize;
	float height = heightmap[tileZ * size + tileX];
	if (height > entity.position.y + entity.centerToGroundContactPoint) {
		entity.position.y = height - entity.centerToGroundContactPoint;
		entity.velocity.y = 0;
	}
}

GLuint loadPNGTexture(std::string filename) {
	const char* filenameC = (const char*)filename.c_str();
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);

	std::vector<unsigned char> imageCopy(width * height * 4);
	for (unsigned i = 0; i < height; i++) {
		memcpy(&imageCopy[(height - i - 1) * width * 4], &image[i * width * 4], width * 4);
	}

	GLuint texId;
	glGenTextures(1, &texId);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texId);
	glActiveTexture(GL_TEXTURE0);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageCopy[0]);
	glGenerateMipmap(GL_TEXTURE_2D);
	return texId;
}

GLuint getShader() {
	std::string vertexSource = readFile("Source\\vertexShader.glsl");
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* vertexSourceC = (const GLchar *)vertexSource.c_str();
	glShaderSource(vertexShader, 1, &vertexSourceC, 0);

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

	std::string fragmentSource = readFile("Source\\fragmentShader.glsl");
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* fragmentSourceC = (const GLchar *)fragmentSource.c_str();
	glShaderSource(fragmentShader, 1, &fragmentSourceC, 0);

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

glm::vec3 cameraPosition = glm::vec3(52.5f, 512, 35.5f);
glm::vec3 cameraForward = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool isForward, isBackward, isLeft, isUp, isRight, isDown, isStrideLeft, isStrideRight, jump;

void basicSteering(glm::vec3 &position, glm::vec3 &forward, glm::vec3 &up) {
	GLfloat rotationSpeed = 0.03f;
	GLfloat speed = 0.25f * 50;
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

void rotateEntity(Entity &entity, glm::vec3 axis, float amount) {
	entity.up = glm::normalize( glm::rotate(entity.up, amount, axis) );
	entity.forward = glm::normalize( glm::rotate(entity.forward, amount, axis) );
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
	cameraPosition = cameraPosition + direction * dt * glm::length(targetPosition - cameraPosition) / 1.0f * 8.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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

int main() {
	const int windowHeight = 900;
	const int windowWidth = 1700;
	glm::mat4 perspective = glm::perspective<GLfloat>(0.8f, windowWidth / (float)windowHeight, .1f, 100000);

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

	GLuint shaderProgram = getShader();
	glUseProgram(shaderProgram);

	const int size = 2049;
	const int tileSizeXZ = 32;
	const int tileSizeY = 1;
	time_t seed = 1519128009; // Splat map is built after this seed, so don't change it
	float *heightmapData = new float[size * size];
	diamondSquare(heightmapData, size, 0.05f, seed);
	makeRunwayOnHeightmap(heightmapData, size);
	Model terrain = heightmapToModel(heightmapData, size, size, tileSizeXZ, tileSizeY, tileSizeXZ, 100);

	Terrain ground = Terrain();
	ground.vao = terrain.vao;
	ground.numIndices = terrain.numIndices;
	ground.position = glm::vec3(0, 0, 0);
	ground.scale = glm::vec3(1, 1, 1);
	ground.textureId = loadPNGTexture("Resources/grass512.png");
	ground.setTextureId2(loadPNGTexture("Resources/sand512.png"));
	ground.setTextureId3(loadPNGTexture("Resources/rock512.png"));
	ground.setTextureId4(loadPNGTexture("Resources/terrain-splatmap.png"));

	Model m = tinyObjLoader("Resources/jas.obj");
	Entity plane = Entity();
	plane.vao = m.vao;
	plane.textureId = loadPNGTexture("Resources/jas.png");
	plane.numIndices = m.numIndices;
	plane.position = glm::vec3((size * tileSizeXZ)/(float)2, 112, (size * tileSizeXZ) / (float)2);
	plane.scale = glm::vec3(0.5f, 0.5f, 0.5f);
	plane.centerToGroundContactPoint = -1;

	glClearColor(0.27, 0.43, 0.66, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	float lastTime = glfwGetTime();
	float currentTime;
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		float currentTime = glfwGetTime();
		float dt = currentTime - lastTime;
		lastTime = currentTime;
		basicSteering(cameraPosition, cameraForward, cameraUp);

		airplanePhysics(plane.position, plane.forward, plane.up, plane.velocity, isForward ? 1 : (isBackward ? -1 : 0), isLeft ? 1 : (isRight ? -1 : 0), isDown ? 1 : (isUp ? -1 : 0), dt);
		terrainCollision(heightmapData, size, tileSizeXZ, plane);

		// Normal camera
		glm::mat4 cam = glm::lookAt(cameraPosition, cameraPosition + cameraForward * 10.0f, cameraUp);

		//glm::vec3 targetposition = plane.position + -plane.forward * 7.5f + plane.up * 3.0f;
		//interpolatecamera(targetposition, cameraposition);
		//cam = glm::lookat(cameraposition, plane.position, plane.up);

		// Render entities
		renderTerrain(ground, shaderProgram, cam, perspective);
		renderEntity(plane, shaderProgram, cam, perspective);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete heightmapData;

	return 0;
}