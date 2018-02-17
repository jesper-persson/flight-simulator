#include "Common.h"
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

std::string readFile(std::string path) {
	std::ifstream t(path);
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return str;
}

Model heightmapToModel(float *heightmap, int width, int height, float scaleX, float scaleY, float scaleZ, float textureScale) {
	std::vector<float> positionsV;
	std::vector<float> normalsV;
	std::vector<float> textureCoordinatesV;
	std::vector<int> indicesV;

	// Populate positions, normals and textureCoordinats
	for (int z = 0; z < height; z++) {
		for (int x = 0; x < width; x++) {
			positionsV.push_back(x * scaleX);
			positionsV.push_back(heightmap[width * z + x] * scaleY);
			positionsV.push_back(z * scaleZ);
			textureCoordinatesV.push_back(((float)x / (float)(width - 1)) * textureScale);
			textureCoordinatesV.push_back((1 - (float)z / (float)(height - 1)) * textureScale);

			// Calc normals
			if (x == width - 1 || z == height - 1 || x == 0 || z == 0) {
				normalsV.push_back(0);
				normalsV.push_back(1);
				normalsV.push_back(0);
			} else {
				glm::vec3 ab = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3((x + 1) * scaleX, heightmap[width * z + x + 1] * scaleY, z * scaleZ);
				glm::vec3 ac = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3(x * scaleX, heightmap[width * (z + 1) + x] * scaleY, (z + 1) * scaleZ);
				glm::vec3 normal1 = glm::normalize(glm::cross(ac, ab));
				glm::vec3 ad = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3((x - 1) * scaleX, heightmap[width * z + x - 1] * scaleY, z * scaleZ);
				glm::vec3 ae = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3(x * scaleX, heightmap[width * (z - 1) + x] * scaleY, (z - 1) * scaleZ);
				glm::vec3 normal2 = glm::normalize(glm::cross(ae, ad));
				glm::vec3 af = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3((x - 1) * scaleX, heightmap[width * z + x - 1] * scaleY, z * scaleZ);
				glm::vec3 ag = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3(x * scaleX, heightmap[width * (z + 1) + x] * scaleY, (z + 1) * scaleZ);
				glm::vec3 normal3 = glm::normalize(glm::cross(af, ag));
				glm::vec3 ah = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3((x + 1) * scaleX, heightmap[width * z + x + 1] * scaleY, z * scaleZ);
				glm::vec3 ai = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3(x * scaleX, heightmap[width * (z - 1) + x] * scaleY, (z - 1) * scaleZ);
				glm::vec3 normal4 = glm::normalize(glm::cross(ah, ai));
				glm::vec3 normal = glm::normalize(normal1 + normal2 + normal3 + normal4);
				normalsV.push_back(normal.x);
				normalsV.push_back(normal.y);
				normalsV.push_back(normal.z);
			}

			// Indices
			if (x != width - 1 && z != height - 1) {
				int index = z * width + x;
				indicesV.push_back(index);
				indicesV.push_back(index + 1);
				indicesV.push_back(index + width);
				indicesV.push_back(index + 1);
				indicesV.push_back(index + width + 1);
				indicesV.push_back(index + width);
			}
		}
	}

	float *positions = &positionsV[0];
	float *normals = &normalsV[0];
	float *textureCoordinates = &textureCoordinatesV[0];
	int *indices = &indicesV[0];

	return modelFromVertexData(positions, width * height * 3,
							  normals, width * height * 3,
							  textureCoordinates, width * height * 2,
							  indices, (width - 1) * (height - 1) * 6);
}

// Sizes given in amounts
Model modelFromVertexData(float vertexCoordinates[], int vertexCoordinatesSize, float normals[], int normalsSize, float textureCoordinates[], int textureCoordinatesSize, int indices[], int indicesSize) {
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Vertex
	GLuint vertexBuffer = 0;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexCoordinatesSize, vertexCoordinates, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// Normals
	GLuint normalBuffer = 0;
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * normalsSize, normals, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	// Texture
	GLuint textureBuffer = 0;
	glGenBuffers(1, &textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * textureCoordinatesSize, textureCoordinates, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	// Index
	GLuint indexBuffer = 0;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indicesSize, indices, GL_STATIC_DRAW);

	Model m;
	m.vao = vao;
	m.numIndices = indicesSize;
	return m;
}