#pragma once

#include "glm\vec3.hpp"

#include "Common.h"

class Particle {
public:
	Particle(glm::vec3 position, glm::vec3 scale, glm::vec3 velocity, float lifetime) {
		this->position = position;
		this->velocity = velocity;
		this->scale = scale;
		this->lifetime = lifetime;
	}
	Particle() {

	}
	void setParentEntity(Entity *parentEntity) {
		this->parentEntity = parentEntity;
	}
	Entity *getParentEntity() {
		return this->parentEntity;
	}
	Entity *parentEntity;
	glm::vec3 scale;
	glm::vec3 position;
	float rotation = 0;
	glm::vec3 velocity;
	// Denotes the duration in seconds this particle has lived
	float timeAlive = 0;
	// Denotes the duration in seconds this particle should live
	float lifetime;
};

class ParticleSystem {
public:
	ParticleSystem(int maxNumParticles) {
		this->maxNumParticles = maxNumParticles;
		particles = new Particle[maxNumParticles];
	}
	~ParticleSystem() {
		delete particles;
	}
	void setDirection(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
		this->minX = minX;
		this->maxX = maxX;
		this->minY = minY;
		this->maxY = maxY;
		this->minZ = minZ;
		this->maxZ = maxZ;
	}
	glm::vec3 position;
	float velocity = 0;
	Model model;
	int textureId;
	int atlasSize;
	int particlesPerSecond;
	int maxNumParticles;
	int numParticles;
	float timeSinceLastSpawn;
	float minLifetime;
	float maxLifetime;
	float minSize = 1;
	float maxSize = 1;
	float sphereRadiusSpawn = 1;
	Particle *particles;
	float minX, maxX, minZ, maxZ, minY, maxY;
	Entity *parentEntity;
	// Used for spawning particles in between frames
	glm::vec3 parentEntityLastPosition = glm::vec3(0, 0, 0);
	bool followParent = false;
};

// cameraPosition and cameraDirection is needed for depth sorting
void updateParticleSystem(ParticleSystem &particleSystem, glm::vec3 cameraPosition, glm::vec3 cameraDirection, float dt);