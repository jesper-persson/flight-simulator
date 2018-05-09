#include <algorithm>

#include <glm/gtc/type_ptr.hpp> 

#include "ParticleSystem.h"

glm::vec3 generateParticleDirection(ParticleSystem &particleSystem) {
	float xMin = particleSystem.minX;
	float xMax = particleSystem.maxX;
	float yMin = particleSystem.minY;
	float yMax = particleSystem.maxY;
	float zMin = particleSystem.minZ;
	float zMax = particleSystem.maxZ;

	float scale = 100;
	float x = (random(0, abs(xMax - xMin) * scale) + xMin * scale) / scale;
	float y = (random(0, abs(yMax - yMin) * scale) + yMin * scale) / scale;
	float z = (random(0, abs(zMax - zMin) * scale) + zMin * scale) / scale;

	glm::vec3 direction = glm::normalize(glm::vec3(x, y, z));
	return direction;
}

void sortParticles(ParticleSystem &particleSystem, Particle *particle, int numParticles, glm::vec3 cameraPosition, glm::vec3 cameraForward) {
	if (numParticles == 0) {
		return;
	}

	LARGE_INTEGER startingTime, endingTime, elapsedMicroseconds;
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&startingTime);

	// Rembember that parentTransformation will be column major
	const float *parentTransformation = nullptr;
	if (particle->getParentEntity() && particleSystem.followParent) {
		parentTransformation = glm::value_ptr(getEntityTransformation(*particle->getParentEntity()));
	}

	std::pair<float, int> *sortArray = new std::pair<float, int>[numParticles];
	Particle *particleIterator = particle;
	glm::vec3 position;
	glm::vec3 distance;
	for (int i = 0; i < numParticles; i++) {
		if (parentTransformation) {
			position.x = parentTransformation[0] * particleIterator->position.x + parentTransformation[4] * particleIterator->position.y + parentTransformation[8] * particleIterator->position.z + parentTransformation[12];
			position.y = parentTransformation[1] * particleIterator->position.x + parentTransformation[5] * particleIterator->position.y + parentTransformation[9] * particleIterator->position.z + parentTransformation[13];
			position.z = parentTransformation[2] * particleIterator->position.x + parentTransformation[6] * particleIterator->position.y + parentTransformation[10] * particleIterator->position.z + parentTransformation[14];
		}
		else {
			position = particleIterator->position;
		}

		distance = position - cameraPosition;
		sortArray[i].first = distance.x * cameraForward.x + distance.y * cameraForward.y + distance.z * cameraForward.z;
		sortArray[i].second = i;

		particleIterator++;
	}

	Particle *particleCopy = new Particle[numParticles];
	for (int i = 0; i < numParticles; i++) {
		particleCopy[i] = particle[i];
	}

	std::pair<float, int> *last = &sortArray[numParticles - 1];
	std::sort(sortArray, last, [](std::pair<float, int> const &a, std::pair<float, int> const  &b) -> bool {
		return a.first > b.first;
	});

	for (int i = 0; i < numParticles; i++) {
		particle[i] = particleCopy[sortArray[i].second];
	}

	delete[] particleCopy;
	delete[] sortArray;

	QueryPerformanceCounter(&endingTime);
	elapsedMicroseconds.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
	elapsedMicroseconds.QuadPart *= 1000000;
	elapsedMicroseconds.QuadPart /= frequency.QuadPart;
	std::cout << elapsedMicroseconds.QuadPart << std::endl;
}

void updateParticle(ParticleSystem &particleSystem, Particle &particle, float dt) {
	particle.position = particle.position + particle.velocity * dt;
	particle.timeAlive += dt;
}

void updateParticleSystem(ParticleSystem &particleSystem, glm::vec3 cameraPosition, glm::vec3 cameraDirection, float dt) {
	// Update particles
	for (int i = 0; i < particleSystem.numParticles; i++) {
		updateParticle(particleSystem, particleSystem.particles[i], dt);
		if (particleSystem.particles[i].timeAlive >= particleSystem.particles[i].lifetime) {
			particleSystem.numParticles -= 1;
			if (particleSystem.numParticles > 0) {
				particleSystem.particles[i] = particleSystem.particles[particleSystem.numParticles];
				i--;
			}
		}
	}

	// Spawn new particles
	particleSystem.timeSinceLastSpawn += dt;
	int numParticlesToSpawn = (int)particleSystem.particlesPerSecond * particleSystem.timeSinceLastSpawn;
	glm::mat4 parentTransformation = glm::mat4();
	if (particleSystem.parentEntity) {
		parentTransformation = getEntityTransformation(*particleSystem.parentEntity);
	}
	glm::vec3 parentPosition = parentTransformation[3];
	glm::vec3 oldParentPositionDir = (parentPosition - particleSystem.parentEntityLastPosition);
	glm::vec3 newOldPosition = parentTransformation[3];

	for (int i = 0; i < numParticlesToSpawn && particleSystem.numParticles < particleSystem.maxNumParticles; i++) {
		Particle p = Particle();
		float scale = random(particleSystem.minSize * 100, particleSystem.maxSize * 100) / 100.0f;
		p.scale = glm::vec3(scale, scale, 1.0f);
		p.lifetime = random(particleSystem.minLifetime * 100, particleSystem.maxLifetime * 100) / 100.0f;
		p.velocity = generateParticleDirection(particleSystem) * particleSystem.velocity;
		p.position = /*glm::normalize(p.velocity) * particleSystem.sphereRadiusSpawn */ /** dt * (float)i/(float)numParticlesToSpawn*/  particleSystem.position;
		p.rotation = random(0, 3.14 * 100) / 100.0f;
		if (!particleSystem.followParent) {
			parentTransformation[3] = glm::vec4(parentPosition + oldParentPositionDir * (float)(i) / (float)numParticlesToSpawn, 1);
			p.position = glm::vec3(parentTransformation * glm::vec4(p.position.x, p.position.y, p.position.z, 1));
		}
		p.setParentEntity(particleSystem.parentEntity);
		particleSystem.particles[particleSystem.numParticles] = p;
		particleSystem.numParticles += 1;
		particleSystem.timeSinceLastSpawn = 0;
	}
	particleSystem.parentEntityLastPosition = newOldPosition;

	// Sort particles

	sortParticles(particleSystem, particleSystem.particles, particleSystem.numParticles, cameraPosition, cameraDirection);
}