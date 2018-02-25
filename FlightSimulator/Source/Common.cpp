#include "Common.h"
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include <glm/gtx/transform.hpp> 
#include <glm/gtx/rotate_vector.hpp> 

#include <memory>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Entity::Entity() {
	position = glm::vec3(0, 0, 0);
	scale = glm::vec3(1, 1, 1);
	velocity = glm::vec3(0, 0, 0);
	centerToGroundContactPoint = 0;
	impulse = glm::vec3(0, 0, 0);
	rotationPivot = glm::vec3(0, 0, 0);
	forward = DEFAULT_FORWARD;
	up = DEFAULT_UP;
	parentEntity = nullptr;
	name = "";
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

Model tinyObjLoader(std::string filename) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());

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

std::vector<Entity*> loadJAS39Gripen(std::string filename) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> textures;
	std::vector<int> indices;

	std::vector<Entity*> entities;

	// Loop over shapes
	int i = 0;
	for (size_t s = 0; s < shapes.size(); s++) {
		std::cout << shapes[s].name << std::endl;

		Entity *entity = new Entity();
		entity->setName(shapes[s].name);
		Model m = Model();
		m.offset = i;

		if (shapes[s].name == "AileronL01") {
			entity->setRotationPivot(glm::vec3(3.265f, -0.16f, -3.58));
		}
		if (shapes[s].name == "AileronR01") {
			entity->setRotationPivot(glm::vec3(-3.265f, -0.16f, -3.58));
		}
		if (shapes[s].name == "FlapL01") {
			entity->setRotationPivot(glm::vec3(2.265f, -0.16f, -3.58));
		}
		if (shapes[s].name == "FlapR01") {
			entity->setRotationPivot(glm::vec3(-2.265f, -0.16f, -3.58));
		}
		
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
	
		m.numIndices = i - m.offset;
		entity->setModel(m);
		entities.push_back(entity);
	}

	float *vertexData = &vertices[0];
	float *normalData = &normals[0];
	float *textureData = &textures[0];
	int *indexData = &indices[0];

	Model completeModel = modelFromVertexData(vertexData, vertices.size(), normalData, normals.size(), textureData, textures.size(), indexData, indices.size());
	
	GLfloat *tangentData = new GLfloat[vertices.size()];
	GLfloat *bitangentData = new GLfloat[vertices.size()];
	calculateTangents(vertexData, textureData, vertices.size() / 3, tangentData, bitangentData);

	GLuint tangentBuffer = 0;
	glGenBuffers(1, &tangentBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * vertices.size() / 3, tangentData, GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(3);

	GLuint bitangentBuffer = 0;
	glGenBuffers(1, &bitangentBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, bitangentBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * vertices.size() / 3, bitangentData, GL_STATIC_DRAW);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(4);

	delete tangentData;
	delete bitangentData;

	for (std::vector<Entity*>::iterator iter = entities.begin(); iter != entities.end(); iter++) {
		(*iter)->getModel().vao = completeModel.vao;
		if (iter != entities.begin()) {
			(*iter)->setParentEntity(entities[0]);
		}
	}

	return std::move(entities);
}

void rotateEntity(Entity &entity, glm::vec3 axis, float amount) {
	entity.up = glm::normalize(glm::rotate(entity.up, amount, axis));
	entity.forward = glm::normalize(glm::rotate(entity.forward, amount, axis));
}

Model getVAOCube() {
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	float vertexData[] = {
		-1, -1, 1, // Front
		-1, 1, 1,
		1, -1, 1,

		1, 1, 1, // Front
		1, -1, 1,
		-1, 1, 1,

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

	GLfloat normalData[] = {
		0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
		0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
		-1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
		1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
		0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
		0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0
	};

	GLfloat textureData[] = {
		0, 0, 0, 1, 1, 0,
		1, 1, 1, 0, 0, 1,

		1, 0, 1, 1, 0, 0,
		0, 1, 1, 1, 0, 0,

		0, 1, 0, 0, 1, 0,
		0, 1, 1, 1, 1, 0,
		
		0, 1, 0, 0, 1, 0,
		0, 1, 1, 1, 1, 0,
		
		0, 1, 0, 0, 1, 0,
		0, 1, 1, 1, 1, 0,

		0, 1, 0, 0, 1, 0,
		0, 1, 1, 1, 1, 0,
	};

	GLuint indexData[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 ,16 ,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35
	};

	GLfloat tangentData[36 * 3];
	GLfloat bitangentData[36 * 3];
	calculateTangents(vertexData, textureData, 36, tangentData, bitangentData);

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

	GLuint textureBuffer = 0;
	glGenBuffers(1, &textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(GLfloat) * 36, textureData, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	GLuint tangentBuffer = 0;
	glGenBuffers(1, &tangentBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 36, tangentData, GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(3);

	GLuint bitangentBuffer = 0;
	glGenBuffers(1, &bitangentBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, bitangentBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 36, bitangentData, GL_STATIC_DRAW);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(4);

	GLuint indexBuffer = 0;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 36, indexData, GL_STATIC_DRAW);

	Model model = Model();
	model.vao = vao;
	model.offset = 0;
	model.numIndices = 36;

	return model;
}

// assumes three vertices per face
void calculateTangents(float *vertexData, float *textureData, int numVertices, float *tangentData, float *bitangentData) {
	for (int i = 0; i < numVertices * 3; i += 9) { // Once per triangle
		glm::vec3 a = glm::vec3(vertexData[i], vertexData[i + 1], vertexData[i + 2]);
		glm::vec3 b = glm::vec3(vertexData[i + 3], vertexData[i + 4], vertexData[i + 5]);
		glm::vec3 c = glm::vec3(vertexData[i + 6], vertexData[i + 7], vertexData[i + 8]);

		glm::vec3 deltaPos1 = glm::normalize(b - a);
		glm::vec3 deltaPos2 = glm::normalize(c - a);

		glm::vec2 texCoord1 = glm::vec2(textureData[i / 3 * 2], textureData[i / 3 * 2 + 1]);
		glm::vec2 texCoord2 = glm::vec2(textureData[i / 3 * 2 + 2], textureData[i / 3 * 2 + 3]);
		glm::vec2 texCoord3 = glm::vec2(textureData[i / 3 * 2 + 4], textureData[i / 3 * 2 + 5]);

		glm::vec2 deltaTex1 = glm::normalize(texCoord2 - texCoord1);
		glm::vec2 deltaTex2 = glm::normalize(texCoord3 - texCoord1);

		float r = 1.0f / (deltaTex1.x * deltaTex2.y - deltaTex1.y * deltaTex2.x);
		glm::vec3 tangent = glm::normalize((deltaPos1 * deltaTex2.y - deltaPos2 * deltaTex1.y) * r);
		glm::vec3 bitangent = glm::normalize((deltaPos2 * deltaTex1.x - deltaPos1 * deltaTex2.x) * r);

		tangentData[i] = tangent.x;
		tangentData[i + 1] = tangent.y;
		tangentData[i + 2] = tangent.z;
		tangentData[i + 3] = tangent.x;
		tangentData[i + 4] = tangent.y;
		tangentData[i + 5] = tangent.z;
		tangentData[i + 6] = tangent.x;
		tangentData[i + 7] = tangent.y;
		tangentData[i + 8] = tangent.z;
		bitangentData[i] = bitangent.x;
		bitangentData[i + 1] = bitangent.y;
		bitangentData[i + 2] = bitangent.z;
		bitangentData[i + 3] = bitangent.x;
		bitangentData[i + 4] = bitangent.y;
		bitangentData[i + 5] = bitangent.z;
		bitangentData[i + 6] = bitangent.x;
		bitangentData[i + 7] = bitangent.y;
		bitangentData[i + 8] = bitangent.z;
	}
}