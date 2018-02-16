#include "Common.h"

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

Model objLoader(std::string path) {
	std::ifstream infile(path);
	std::string line;

	std::vector<GLfloat> vertices;
	std::vector<GLfloat> normals;
	std::vector<GLuint> indices;

	while (std::getline(infile, line)) {
		std::vector<std::string> lineParts;
		std::istringstream f(line);

		std::string s;
		while (getline(f, s, ' ')) {
			if (s == "") {
				continue;
			}
			lineParts.push_back(s);
		}

		if (lineParts.size() == 0) {
			continue;
		}

		if (lineParts[0] == "v") {
			vertices.push_back(std::stof(lineParts[1]));
			vertices.push_back(std::stof(lineParts[2]));
			vertices.push_back(std::stof(lineParts[3]));
		}
		if (lineParts[0] == "vn") {
			normals.push_back(std::stof(lineParts[1]));
			normals.push_back(std::stof(lineParts[2]));
			normals.push_back(std::stof(lineParts[3]));
		}
		if (lineParts[0] == "f") {
			for (int i = 1; i <= 3; i++) {
				std::vector<std::string> faceParts;
				std::istringstream ff(lineParts[i]);
				while (getline(ff, s, '/')) {
					faceParts.push_back(s);
				}
				if (faceParts.size() > 0) {
					indices.push_back(std::stoi(faceParts[0]) - 1);
				} else {
					indices.push_back(std::stoi(lineParts[i]) - 1);
				}
			}
		}
	}

	GLfloat *vertexData = &vertices[0];
	GLfloat *normalData = &normals[0];
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

	GLuint indexBuffer = 0;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indexData, GL_STATIC_DRAW);

	Model m;
	m.numIndices = vertices.size();
	m.vao = vao;
	return m;
}
