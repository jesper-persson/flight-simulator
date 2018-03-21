#include "Physics.h"
#include <iostream>
#include <algorithm>

#include <glm/gtx/transform.hpp> 
#include <glm/gtx/rotate_vector.hpp> 

void steerAirplane(Entity &main, Entity &aileronLeft, Entity &aileronRight, Entity &leftFlap, Entity &rightFlap, int thrust, int roll, int pitch, float dt) {
	// Roll
	{
		float rotAmountRoll = -dt * 1.9f;
		if (roll) {
			rotAmountRoll *= (float)roll;
		}
		else {
			rotAmountRoll *= glm::dot(aileronLeft.forward, DEFAULT_UP) > 0 ? 1 : -1;
		}

		glm::vec3 rotAxis = glm::normalize(glm::vec3(1, 0, 0));
		const float maxAngleRoll = 0.7f;

		rotateEntity(aileronLeft, rotAxis, -rotAmountRoll);
		rotateEntity(aileronRight, rotAxis, rotAmountRoll);
		float newAngleRoll = acos(glm::dot(DEFAULT_FORWARD, aileronLeft.forward));
		if (newAngleRoll > maxAngleRoll) {
			aileronLeft.forward = DEFAULT_FORWARD;
			aileronLeft.up = DEFAULT_UP;
			aileronRight.forward = DEFAULT_FORWARD;
			aileronRight.up = DEFAULT_UP;
			rotateEntity(aileronLeft, rotAxis, (float)roll*maxAngleRoll);
			rotateEntity(aileronRight, rotAxis, -(float)roll*maxAngleRoll);
		}
	}
	
	// Pitch
	{
		float rotAmountPitch = -dt * 1.9f;
		if (pitch) {
			rotAmountPitch *= -(float)pitch;
		}
		else {
			rotAmountPitch *= glm::dot(rightFlap.forward, DEFAULT_UP) > 0 ? -1 : 1;
		}

		glm::vec3 rotAxis = glm::normalize(glm::vec3(1, 0, 0));
		const float maxAnglePitch = 0.7f;

		rotateEntity(leftFlap, rotAxis, rotAmountPitch);
		rotateEntity(rightFlap, rotAxis, rotAmountPitch);
		float newAnglePitch = acos(glm::dot(DEFAULT_FORWARD, leftFlap.forward));
		if (newAnglePitch > maxAnglePitch) {
			leftFlap.forward = DEFAULT_FORWARD;
			leftFlap.up = DEFAULT_UP;
			rightFlap.forward = DEFAULT_FORWARD;
			rightFlap.up = DEFAULT_UP;
			rotateEntity(leftFlap, rotAxis, (float)pitch*maxAnglePitch);
			rotateEntity(rightFlap, rotAxis, (float)pitch*maxAnglePitch);
		}
	}

	float maxThrustForce = 40000; // Newtons
	float maxBreakForce = 2000; // Newtons
	glm::vec3 thrustForce = main.forward * (float)thrust * (thrust == 1 ? maxThrustForce : maxBreakForce);
	main.impulse = thrustForce;
}

// dt in seconds
// assumes 1 distance unit in the game is 1 meter
void airplanePhysics(Entity &entity, Entity &aileronLeft, Entity &leftFlap, float dt) {
	float mass = 1000;
	float gravitationalAcceleration = 9.82f;
	float airResistanceConstant = 20; // Includes density and area
	float liftConstant = 160;

	glm::vec3 &up = entity.up;
	glm::vec3 &forward = entity.forward;
	glm::vec3 &velocity = entity.velocity;
	glm::vec3 &position = entity.position;

	glm::vec3 thrustForce = entity.impulse;
	entity.impulse = glm::vec3(0, 0, 0);
	
	glm::vec3 airResitance = glm::vec3(0, 0, 0);
	if (glm::length(velocity) > 0) {
		float areaTopBottom = 1 + 10.3f * std::abs(glm::dot(up, glm::normalize(velocity)));
		float areaLeftRight = 1 + 1.2f * std::abs(glm::dot(glm::normalize(glm::cross(up, forward)), glm::normalize(velocity)));

		airResitance = -glm::normalize(velocity) * glm::length(velocity) * glm::length(velocity) * airResistanceConstant * areaTopBottom * areaLeftRight;
	}

	// Pitch
	float attackAnglePitch = -acos(glm::dot(leftFlap.forward, DEFAULT_FORWARD));
	if (glm::dot(leftFlap.forward, DEFAULT_UP) < 0) { // Lift
		attackAnglePitch = -attackAnglePitch;
	}
	float rotationSpeedPitch = 0.0003f * attackAnglePitch * glm::length(velocity);
	glm::vec3 right = glm::normalize(glm::cross(forward, up));
	rotateEntity(entity, right, rotationSpeedPitch);
	
	// Roll
	float attackAngleRoll = acos(glm::dot(aileronLeft.forward, DEFAULT_FORWARD));
	if (glm::dot(aileronLeft.forward, DEFAULT_UP) < 0) { // Lift
		attackAngleRoll = -attackAngleRoll;
	}
	float rotationSpeedRoll = 0.001f * attackAngleRoll * glm::length(velocity);
	up = glm::normalize(glm::rotate(up, rotationSpeedRoll, forward));


	glm::vec3 liftForce = up * dot(forward, velocity) * (liftConstant);

	glm::vec3 gravity = glm::vec3(0, -mass * gravitationalAcceleration, 0);

	glm::vec3 netForce = gravity + thrustForce + airResitance + liftForce;
	glm::vec3 acceleration = netForce / mass;
	velocity = velocity + acceleration * dt;
	position = position + velocity * dt;
}

void normalPhysics(Entity &entity, float dt) {
	float mass = 1000;
	float gravitationalAcceleration = 9.82f;
	glm::vec3 gravity = glm::vec3(0, -mass * gravitationalAcceleration, 0);
	glm::vec3 airResistance = -glm::normalize(entity.velocity) * glm::length(entity.velocity) * glm::length(entity.velocity) * 520.0f;
	glm::vec3 netForce = gravity + airResistance;
	glm::vec3 acceleration = netForce / mass;
	entity.velocity = entity.velocity + acceleration * dt;
	entity.position = entity.position + entity.velocity * dt;
}