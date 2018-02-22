#include "Common.h"
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include <memory>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Entity::Entity() {
	std::cout << "Körs konstruktor" << std::endl;
	position = glm::vec3(0, 0, 0);
	scale = glm::vec3(1, 1, 1);
	velocity = glm::vec3(0, 0, 0);
	centerToGroundContactPoint = 0;
	impulse = glm::vec3(0, 0, 0);
	forward = DEFAULT_FORWARD;
	up = DEFAULT_UP;
};

std::string readFile(std::string path) {
	std::ifstream t(path);
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return str;
}

Model heightmapToModel(float *heightmap, int width, int height, float scaleX, float scaleY, float scaleZ, float textureScale) {
	std::unique_ptr<float> positions(new float[width * height * 3]);
	std::unique_ptr<float> normals(new float[width * height * 3]);
	std::unique_ptr<float> textureCoordinates(new float[width * height * 2]);
	std::unique_ptr<int> indices(new int[(width - 1) * (height - 1) * 6]);

	for (int z = 0; z < height; z++) {
		for (int x = 0; x < width; x++) {
			positions.get()[(z * width + x) * 3] = x * scaleX;
			positions.get()[(z * width + x) * 3 + 1] = heightmap[width * z + x] * scaleY;
			positions.get()[(z * width + x) * 3 + 2] = z * scaleZ;
			textureCoordinates.get()[(z * width + x) * 2] = ((float)x / (float)(width - 1)) * textureScale;
			textureCoordinates.get()[(z * width + x) * 2 + 1] = (1 - (float)z / (float)(height - 1)) * textureScale;

			// Calc normals
			if (x == width - 1 || z == height - 1 || x == 0 || z == 0) {
				normals.get()[(z * width + x) * 3] = 0;
				normals.get()[(z * width + x) * 3 + 1] = 1;
				normals.get()[(z * width + x) * 3 + 2] = 0;
			} else {
				glm::vec3 ab = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3((x + 1) * scaleX, heightmap[width * z + x + 1] * scaleY, z * scaleZ);
				glm::vec3 ac = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3(x * scaleX, heightmap[width * (z + 1) + x] * scaleY, (z + 1) * scaleZ);
				glm::vec3 normal1 = glm::normalize(glm::cross(ac, ab));
				//glm::vec3 ad = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3((x - 1) * scaleX, heightmap[width * z + x - 1] * scaleY, z * scaleZ);
				//glm::vec3 ae = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3(x * scaleX, heightmap[width * (z - 1) + x] * scaleY, (z - 1) * scaleZ);
				//glm::vec3 normal2 = glm::normalize(glm::cross(ae, ad));
				//glm::vec3 af = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3((x - 1) * scaleX, heightmap[width * z + x - 1] * scaleY, z * scaleZ);
				//glm::vec3 ag = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3(x * scaleX, heightmap[width * (z + 1) + x] * scaleY, (z + 1) * scaleZ);
				//glm::vec3 normal3 = glm::normalize(glm::cross(af, ag));
				//glm::vec3 ah = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3((x + 1) * scaleX, heightmap[width * z + x + 1] * scaleY, z * scaleZ);
				//glm::vec3 ai = glm::vec3(x * scaleX, heightmap[width * z + x] * scaleY, z * scaleZ) - glm::vec3(x * scaleX, heightmap[width * (z - 1) + x] * scaleY, (z - 1) * scaleZ);
				//glm::vec3 normal4 = glm::normalize(glm::cross(ah, ai));
				//glm::vec3 normal = glm::normalize(normal1 + normal2 + normal3 + normal4);
				normals.get()[(z * width + x) * 3] = normal1.x;
				normals.get()[(z * width + x) * 3 + 1] = normal1.y;
				normals.get()[(z * width + x) * 3 + 2] = normal1.z;
			}

			// Indices
			if (x != width - 1 && z != height - 1) {
				int index = z * width + x;
				int arrayIndex = z * (width - 1) + x;
				indices.get()[arrayIndex * 6] = index;
				indices.get()[arrayIndex * 6 + 1] = index + 1;
				indices.get()[arrayIndex * 6 + 2] = index + width;
				indices.get()[arrayIndex * 6 + 3] = index + 1;
				indices.get()[arrayIndex * 6 + 4] = index + width + 1;
				indices.get()[arrayIndex * 6 + 5] = index + width;
			}
		}
	}

	return modelFromVertexData(positions.get(), width * height * 3,
							  normals.get(), width * height * 3,
							  textureCoordinates.get(), width * height * 2,
							  indices.get(), (width - 1) * (height - 1) * 6);
}

// Sizes given in amounts (that is size of the array)
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

Model tinyObjLoader(std::string fileName) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName.c_str());

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> textures;
	std::vector<int> indices;

	// Loop over shapes
	int i = 0;
	for (size_t s = 0; s < shapes.size(); s++) {
		std::cout << shapes[s].name << std::endl;

		// Loop over faces (polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
				textures.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
				textures.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
				indices.push_back(i);
				i++;
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}

	float *vertexData = &vertices[0];
	float *normalData = &normals[0];
	float *textureData = &textures[0];
	int *indexData = &indices[0];

	return modelFromVertexData(vertexData, vertices.size(), normalData, normals.size(), textureData, textures.size(), indexData, indices.size());
}

Entity *loadJAS39Gripen(std::string filename) {
	return nullptr;
}