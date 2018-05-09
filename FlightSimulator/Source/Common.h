#include <string>
#include <glad\glad.h>

#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <vector>

#ifndef COMMON_H
#define COMMON_H

// Use this to generate smaller worlds, lower textures and such to reduce launch time
const bool FAST_MODE = false;

const glm::vec3 DEFAULT_FORWARD = glm::vec3(0, 0, 1);
const glm::vec3 DEFAULT_UP = glm::vec3(0, 1, 0);

struct Model {
public:
	Model() : offset(0) {
		offset = 0;
		Ns = 0;
		Ni = 1;
		d = 1;
		illum = 0;
		map_Kd = -1;
		Ka = glm::vec3(0, 0, 0);
		Kd = glm::vec3(0, 0, 0);
		Ks = glm::vec3(0, 0, 0);
		color = glm::vec4(0, 0, 0, 0);
	}
	GLuint vao;
	int offset;
	int numIndices;
	float Ns; // Specular expondent
	float Ni; // Optical density
	float d; // Transparencey (0 to 1)
	int illum; // Illumination model
	// Pointer so that we can indicate no value for missing map_Kd:s
	GLuint map_Kd;
	glm::vec3 Ka; // Ambient reflectivity
	glm::vec3 Kd; // Diffuse reflectivity
	glm::vec3 Ks; // Specular reflectivity
	glm::vec4 color;
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
	Entity *getParentEntity() const {
		return parentEntity;
	}
	void setParentEntity(Entity *parentEntity) {
		this->parentEntity = parentEntity;
	}
	glm::vec3 getRotationPivot() const {
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

glm::quat directionToQuaternion(glm::vec3 forward, glm::vec3 up, glm::vec3 defaultForward, glm::vec3 defaultUp);
glm::mat4 getEntityTransformation(Entity const &entity);

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

enum LightType { DIRECTIONAL_LIGHT, POINT_LIGHT, SPOTLIGHT };

// When setting position, use position from Entity
// When setting direction for spotlights, use forward from Entity
class Light : public Entity {
public:
	LightType lightType;
	glm::vec3 color;
	float cutoffAngle;
	float intensity;
	float attenuationC1;
	float attenuationC2;
};


std::string readFile(std::string path);

Model heightmapToModel(float *heightmap, int width, int height, float scaleX, float scaleY, float scaleZ, float textureScale);

Model modelFromVertexData(float vertexCoordinates[], int vertexCoordinatesSize, float normals[], int normalsSize, float textureCoordinates[], int textureCoordinatesSize, int indices[], int indicesSize);

Model tinyObjLoader(std::string fileName);

std::vector<Entity*> loadJAS39Gripen(std::string filename);

void rotateEntity(Entity &entity, glm::vec3 axis, float amount);

Model getVAOCube();
Model getVAOQuad();

void calculateTangents(float *vertexData, float *textureData, int numVertices, float *tangentData, float *bitangentData);

GLuint loadPNGTexture(std::string filename);

int random(int min, int max);

int sign(float i);

std::vector<unsigned char> loadPNG(std::string filename);

#endif