
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

const glm::vec3 defaultForward = glm::vec3(0, 0, 1); // Depends on model
const glm::vec3 defaultUp = glm::vec3(0, 1, 0); // Depends on model

class Entity {
public:
	Entity() {
		position = glm::vec3(0, 0, 0);
		scale = glm::vec3(1, 1, 1);
		velocity = glm::vec3(0, 0, 0);
		groundContactPoint = glm::vec3(0, 0, 0);
		impulse = glm::vec3(0, 0, 0);
		forward = defaultForward;
		up = defaultUp;
	}
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 forward;
	glm::vec3 up;
	glm::vec3 velocity;
	// Distance vector from center of object to point with lowest y value
	glm::vec3 groundContactPoint;
	glm::vec3 impulse;
	GLuint vao;
	int numIndices;
};

glm::quat directionToQuaternion(glm::vec3 forward, glm::vec3 up, glm::vec3 defaultForward, glm::vec3 defaultUp) {
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
	glm::quat quaternion = directionToQuaternion(entity.forward, entity.up, defaultForward, defaultUp);
	glm::mat4 rotation = glm::toMat4(quaternion);

	glm::mat4 transformation = translation * rotation * scale;
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "worldToView"), 1, GL_FALSE, glm::value_ptr(worldToView));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(perspective));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelToWorld"), 1, GL_FALSE, glm::value_ptr(transformation));
	glBindVertexArray(entity.vao);

	glDrawElements(GL_TRIANGLES, entity.numIndices, GL_UNSIGNED_INT, (void*)0);
}

void runPhysics(Entity &ground, Entity &entity, float dt) {
	glm::vec3 netForce =  entity.impulse;
	entity.impulse = glm::vec3(0, 0, 0);
	entity.velocity = entity.velocity + netForce;
	entity.position = entity.position + entity.velocity * dt;
	if (ground.position.y > entity.position.y + entity.groundContactPoint.y) {
		entity.position.y = ground.position.y - entity.groundContactPoint.y;
		entity.velocity.y = 0;
	}
}

GLuint getVAOGround() {
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	float vertexData[] = {
		-1, 0, -1,
		-1, 0, 1,
		1, 0, 1,
		-1, 0, -1,
		1, 0, -1,
		1, 0, 1,
	};

	float normalData[] = {
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
	};

	GLuint textureData[] = {
		0, 1,
		0, 0,
		1, 0,
		0, 1,
		1, 1,
		1, 0 
	};

	GLuint indexData[] = {
		0, 1, 2, 3, 4, 5
	};

	// Vertex
	GLuint vertexBuffer = 0;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 6, vertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// Normals
	GLuint normalBuffer = 0;
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 6, normalData, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	// Texture
	GLuint textureBuffer = 0;
	glGenBuffers(1, &textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(GLuint) * 6, textureData, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_UNSIGNED_INT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	// Index
	GLuint indexBuffer = 0;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 36, indexData, GL_STATIC_DRAW);

	std::string filename = "Resources\\grass512.png";
	const char* filenameC = (const char*)filename.c_str();
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);

	std::vector<unsigned char> imageCopy;
	for (int t = 511; t >= 0; t--) {
		for (int s = 0; s < 512; s++) {
			imageCopy.push_back(image[t * 4 * 512 + 4 * s ]);
			imageCopy.push_back(image[t * 4 * 512 + 4 * s + 1]);
			imageCopy.push_back(image[t * 4 * 512 + 4 * s + 2]);
			imageCopy.push_back(image[t * 4 * 512 + 4 * s + 3]);
		}
	}

	glEnable(GL_TEXTURE_2D);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageCopy[0]);

	return vao;
}

GLuint getVAOTriangle() {
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	float vertexData[] = {
		-1, 0, -2,
		0, 1, -2,
		1, 0, -2,
		-1, 0, 0,
		0, 1, 0,
		1, 0, 0,
		0, 0, 2,
		0, 0, 0,
	};

	float normalData[] = {
		-1, 1, 0,
		0, 1, 0,
		1, 1, 0,
		-1, 1, 0,
		0, 1, 0,
		1, 1, 0,
		0, 1, 0,
		0, -1, 0,
	};
	
	GLuint indexData[] = {
		0, 1, 2,
		0, 2, 3,
		2, 3, 5,
		0, 1, 3,
		1, 3, 4,
		1, 2, 4,
		2, 4, 5,
		3, 4, 6,
		4, 5, 6,
		3, 7, 6,
		5, 6, 7
	};

	GLuint vertexBuffer = 0;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 8, vertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	GLuint normalBuffer = 0;
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 8, normalData, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	GLuint indexBuffer = 0;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 11 * 3, indexData, GL_STATIC_DRAW);

	return vao;
}

GLuint getVAOBox() {
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	float vertexData[] = {
		-1, -1, 1, // Front
		-1, 1, 1,
		1, -1, 1,

		1, 1, 1, // Front
		-1, 1, 1,
		1, -1, 1,

		-1, -1, -1, // Back
		-1, 1, -1,
		1, -1, -1,

		1, 1, -1, // Back
		-1, 1, -1,
		1, -1, -1,

		-1, 1, -1, // Left
		-1, -1, -1,
		-1, -1, 1,

		-1, 1, -1, // Left
		-1, 1, 1,
		-1, -1, 1,

		1, 1, -1, // Right
		1, -1, -1,
		1, -1, 1,

		1, 1, -1, // Right
		1, 1, 1,
		1, -1, 1,

		-1, 1, -1, // Top
		-1, 1, 1,
		1, 1, 1,

		-1, 1, -1, // Top
		1, 1, -1,
		1, 1, 1,

		-1, -1, -1, // Bottom
		-1, -1, 1,
		1, -1, 1,

		-1, -1, -1, // Bottom
		1, -1, -1,
		1, -1, 1,
	};

	float normalData[] = {
		0, 0, 1, // Front
		0, 0, 1,
		0, 0, 1,

		0, 0, 1, // Front
		0, 0, 1,
		0, 0, 1,

		0, 0, -1, // Back
		0, 0, -1,
		0, 0, -1,

		0, 0, -1, // Back
		0, 0, -1,
		0, 0, -1,

		-1, 0, 0, // Left
		-1, 0, 0,
		-1, 0, 0,

		-1, 0, 0, // Left
		-1, 0, 0,
		-1, 0, 0,

		1, 0, 0, // Right
		1, 0, 0,
		1, 0, 0,

		1, 0, 0, // Right
		1, 0, 0,
		1, 0, 0,

		0, 1, 0, // Top
		0, 1, 0,
		0, 1, 0,

		0, 1, 0, // Top
		0, 1, 0,
		0, 1, 0,

		0, -1, 0, // Bottom
		0, -1, 0,
		0, -1, 0,

		0, -1, 0, // Bottom
		0, -1, 0,
		0, -1, 0,
	};

	GLuint indexData[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 ,16 ,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35
	};

	GLuint vertexBuffer = 0;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 36, vertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	GLuint normalBuffer = 0;
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 36, normalData, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);


	GLuint indexBuffer = 0;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 36, indexData, GL_STATIC_DRAW);

	return vao;
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
		std::cout << "Problem compiling shader" << std::endl;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	return program;
}

glm::vec3 cameraPosition = glm::vec3(0, 0, 0);
glm::vec3 cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool isForward, isBackward, isLeft, isUp, isRight, isDown, isStrideLeft, isStrideRight, jump;

void basicSteering(glm::vec3 &position, glm::vec3 &forward, glm::vec3 &up) {
	GLfloat rotationSpeed = 0.03f;
	GLfloat speed = 0.25f;
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

void interpolateCamera(glm::vec3 targetPosition, glm::vec3 &cameraPosition) {
	float dt = 0.01f;
	glm::vec3 direction = targetPosition - cameraPosition;
	cameraPosition = cameraPosition + direction * dt * glm::length(direction);
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


int main() {
	glm::mat4 perspective = glm::perspective<GLfloat>(0.8f, 800.0f/600.0f, .1f, 450);

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	GLFWwindow* window = glfwCreateWindow(800, 600, "Fligt Simulator", NULL, NULL);
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

	Entity ground = Entity();
	ground.vao = getVAOGround();
	ground.numIndices = 6;
	ground.position = glm::vec3(0, -1.0f, 0);
	ground.scale = glm::vec3(4580, 1, 4580);

	Entity box = Entity();
	box.vao = getVAOBox();
	box.numIndices = 36;
	box.position = glm::vec3(10, 2, -3);
	box.scale = glm::vec3(1, 1, 1);
	box.groundContactPoint = glm::vec3(0, -1, 0);

	glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

	float i = 0;
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		basicSteering(cameraPosition, cameraForward, cameraUp);

		i += 0.01f;

		if (jump) {
			jump = false;
			box.impulse = glm::vec3(0, 20, 0);
		}

		runPhysics(ground, box, 0.01f);

		// Normal camera
		glm::mat4 cam = glm::lookAt(
			cameraPosition,
			cameraPosition + cameraForward * 10.0f,
			cameraUp
		);

		// Render entities
		renderEntity(ground, shaderProgram, cam, perspective);
		renderEntity(box, shaderProgram, cam, perspective);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;
}