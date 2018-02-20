
#include <iostream>
#include <vector>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

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
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageCopy[0]);
	glGenerateMipmap(GL_TEXTURE_2D);
	return texId;
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
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	// Index
	GLuint indexBuffer = 0;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 36, indexData, GL_STATIC_DRAW);

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

glm::vec3 cameraPosition = glm::vec3(52.5f, 12, 35.5f);
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

Model tinyObjLoader(std::string fileName) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName.c_str());

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	std::vector<GLfloat> vertices;
	std::vector<GLfloat> normals;
	std::vector<GLfloat> textures;
	std::vector<GLuint> indices;

	// Loop over shapes
	int i = 0;
	for (size_t s = 0; s < shapes.size(); s++) {
		
		// Loop over faces (polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			
			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];

				vertices.push_back(vx);
				vertices.push_back(vy);
				vertices.push_back(vz);
				normals.push_back(nx);
				normals.push_back(ny);
				normals.push_back(nz);
				textures.push_back(tx);
				textures.push_back(ty);
				indices.push_back(i);
				i++;
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}

	GLfloat *vertexData = &vertices[0];
	GLfloat *normalData = &normals[0];
	GLfloat *textureData = &textures[0];
	GLuint *indexData = &indices[0];

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vertexBuffer = 0;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	GLuint normalBuffer = 0;
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normalData, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	GLuint textureBuffer = 0;
	glGenBuffers(1, &textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, textures.size() * sizeof(GLfloat), textureData, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	GLuint indexBuffer = 0;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indexData, GL_STATIC_DRAW);

	Model m;
	m.numIndices = vertices.size();
	m.vao = vao;
	return m;
}

void makeRunwayOnHeightmap(float *heightmap, int size) {
	float avgHeight = heightmap[3 * size + 8] + heightmap[80 * size + 8] / 2.0f;
	for (int x = 3; x <= 8; x++) {
		for (int z = 3; z <= 30; z++) {
			heightmap[z * size + x] = avgHeight + 45.0f;
		}
	}
}

int main() {
	glm::mat4 perspective = glm::perspective<GLfloat>(0.8f, 800.0f/600.0f, .1f, 4000);

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

	//Entity ground = Entity();
	//ground.vao = getVAOGround();
	//ground.numIndices = 6;
	//ground.position = glm::vec3(0, -1.0f, 0);
	//ground.scale = glm::vec3(4580, 1, 4580);
	//ground.textureId = loadPNGTexture("Resources/grass512.png");

	const int size = 257;
	float heightmapData[size * size];
	time_t seed = 1519128009; // Splat map is built after this seed, so don't change it
	diamondSquare(heightmapData, size, 0.5f, seed);
	makeRunwayOnHeightmap(heightmapData, size);
	const int tileSizeXZ = 8;
	const int tileSizeY = 1;
	Model terrain = heightmapToModel(heightmapData, size, size, tileSizeXZ, tileSizeY, tileSizeXZ, 80);
	Terrain ground = Terrain();
	ground.vao = terrain.vao;
	ground.numIndices = terrain.numIndices;
	ground.position = glm::vec3(0, 0, 0);
	ground.scale = glm::vec3(1, 1, 1);
	ground.textureId = loadPNGTexture("Resources/grass512.png");
	ground.setTextureId2(loadPNGTexture("Resources/sand512.png"));
	ground.setTextureId3(loadPNGTexture("Resources/rock512.png"));
	ground.setTextureId4(loadPNGTexture("Resources/terrain-splatmap.png"));

	auto box = Entity();
	box.vao = getVAOBox();
	box.numIndices = 36;
	box.position = glm::vec3(0, 3, 0);
	box.scale = glm::vec3(1, 1, 1);
	box.centerToGroundContactPoint = -0.6;

	Model m = tinyObjLoader("Resources/jas.obj");
	Entity plane = Entity();
	plane.vao = m.vao;
	plane.textureId = loadPNGTexture("Resources/jas.png");
	plane.numIndices = m.numIndices;
	plane.position = glm::vec3(52.5f, 12, 35.5f);
	plane.scale = glm::vec3(0.5f, 0.5f, 0.5f);
	plane.centerToGroundContactPoint = -1;

	glClearColor(41/255.0f, 172/255.0f, 221/255.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float lastTime = glfwGetTime();
	float currentTime;
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		float currentTime = glfwGetTime();
		float dt = currentTime - lastTime;
		lastTime = currentTime;
		//basicSteering(cameraPosition, cameraForward, cameraUp);

		if (jump) {
			jump = false;
			box.impulse = glm::vec3(0, 20, 0);
		}

		runPhysics(ground, box, 0.01f);
		airplanePhysics(plane.position, plane.forward, plane.up, plane.velocity, isForward ? 1 : (isBackward ? -1 : 0), isLeft ? 1 : (isRight ? -1 : 0), isDown ? 1 : (isUp ? -1 : 0), dt);
		terrainCollision(heightmapData, size, tileSizeXZ, plane);
		terrainCollision(heightmapData, size, tileSizeXZ, box);

		// Normal camera
		glm::mat4 cam = glm::lookAt(cameraPosition, cameraPosition + cameraForward * 10.0f, cameraUp);

		interpolateCamera(plane.position + -plane.forward * 7.5f + plane.up * 3.0f, cameraPosition);
		cam = glm::lookAt(cameraPosition, plane.position, plane.up);

		// Render entities
		renderTerrain(ground, shaderProgram, cam, perspective);
		renderEntity(box, shaderProgram, cam, perspective);
		renderEntity(plane, shaderProgram, cam, perspective);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;
}