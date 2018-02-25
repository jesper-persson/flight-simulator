#include <string>
#include <glad\glad.h>
#include <glm\vec3.hpp>
#include <iostream>
#include <vector>

#ifndef COMMON_H
#define COMMON_H

const glm::vec3 DEFAULT_FORWARD = glm::vec3(0, 0, 1);
const glm::vec3 DEFAULT_UP = glm::vec3(0, 1, 0);

struct Model {
public:
	Model() : offset(0) {

	}
	GLuint vao;
	int offset;
	int numIndices;
};

class Entity {
public:
	Entity();
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 forward;
	glm::vec3 up;
	glm::vec3 velocity;
	float centerToGroundContactPoint; // For terrain collision
	glm::vec3 impulse;
	GLuint textureId;
	GLuint normalMapId;
	Model& getModel() {
		return model;
	}
	void setModel(Model model) {
		this->model = model;
	}
	Entity *getParentEntity() {
		return parentEntity;
	}
	void setParentEntity(Entity *parentEntity) {
		this->parentEntity = parentEntity;
	}
	glm::vec3 getRotationPivot() {
		return rotationPivot;
	}
	void setRotationPivot(glm::vec3 rotationPivot) {
		this->rotationPivot = rotationPivot;
	}
	void setName(std::string &name) {
		this->name = name;
	}
	std::string& getName() {
		return name;
	}
private:
	std::string name;
	Entity *parentEntity;
	Model model;
	glm::vec3 rotationPivot;
};

class Terrain : public Entity {
public:
	GLuint getTextureId2() {
		return textureId2;
	}
	GLuint getTextureId3() {
		return textureId3;
	}
	GLuint getTextureId4() {
		return textureId4;
	}
	void setTextureId2(GLuint textureId2) {
		this->textureId2 = textureId2;
	}
	void setTextureId3(GLuint textureId3) {
		this->textureId3 = textureId3;
	}
	void setTextureId4(GLuint textureId4) {
		this->textureId4 = textureId4;
	}
private:
	GLuint textureId2;
	GLuint textureId3;
	GLuint textureId4;
};

std::string readFile(std::string path);

Model heightmapToModel(float *heightmap, int width, int height, float scaleX, float scaleY, float scaleZ, float textureScale);

Model modelFromVertexData(float vertexCoordinates[], int vertexCoordinatesSize, float normals[], int normalsSize, float textureCoordinates[], int textureCoordinatesSize, int indices[], int indicesSize);

Model tinyObjLoader(std::string fileName);

std::vector<Entity*> loadJAS39Gripen(std::string filename);

void rotateEntity(Entity &entity, glm::vec3 axis, float amount);

Model getVAOCube();

void calculateTangents(float *vertexData, float *textureData, int numVertices, float *tangentData, float *bitangentData);

#endif