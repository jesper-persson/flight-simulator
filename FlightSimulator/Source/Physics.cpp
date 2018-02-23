#include "Physics.h"
#include <iostream>
#include <algorithm>

#include <glm/gtx/transform.hpp> 
#include <glm/gtx/rotate_vector.hpp> 

void steerAirplane(Entity &main, Entity &aileronLeft, Entity &aileronRight, Entity &leftFlap, Entity &rightFlap, int thrust, int roll, int pitch, float dt) {
	if (roll) {
		glm::vec3 rotAxis = glm::normalize(glm::vec3(1, 0, 0));
		const float maxAngle = 0.7f;
		float rotAmount = 0.01f * (float)roll;

		rotateEntity(aileronLeft, rotAxis, rotAmount);
		rotateEntity(aileronRight, rotAxis, -rotAmount);
		float newAngle = acos(glm::dot(DEFAULT_FORWARD, aileronLeft.forward));
		if (newAngle > maxAngle) {
			aileronLeft.forward = DEFAULT_FORWARD;
			aileronLeft.up = DEFAULT_UP;
			aileronRight.forward = DEFAULT_FORWARD;
			aileronRight.up = DEFAULT_UP;
			rotateEntity(aileronLeft, rotAxis, (float)roll*maxAngle);
			rotateEntity(aileronRight, rotAxis, -(float)roll*maxAngle);
		}
	}

	if (pitch) {
		glm::vec3 rotAxis = glm::normalize(glm::vec3(1, 0, 0));
		const float maxAngle = 0.7f;
		float rotAmount = 0.01f * (float)pitch;

		rotateEntity(leftFlap, rotAxis, rotAmount);
		rotateEntity(rightFlap, rotAxis, rotAmount);
		float newAngle = acos(glm::dot(DEFAULT_FORWARD, leftFlap.forward));
		if (newAngle > maxAngle) {
			leftFlap.forward = DEFAULT_FORWARD;
			leftFlap.up = DEFAULT_UP;
			rightFlap.forward = DEFAULT_FORWARD;
			rightFlap.up = DEFAULT_UP;
			rotateEntity(leftFlap, rotAxis, (float)pitch*maxAngle);
			rotateEntity(rightFlap, rotAxis, (float)pitch*maxAngle);
		}
	}
}

// dt in seconds
// assumes 1 distance unit in the game is 1 meter
void airplanePhysics(Entity &entity, glm::vec3 &position, glm::vec3 &forward, glm::vec3 &up, glm::vec3 &velocity, int thrust, int roll, int pitch, float dt) {
	float mass = 1000;
	float gravitationalAcceleration = 9.82f;
	float maxThrustForce = 30000; // Newtons
	float maxBreakForce = 2000; // Newtons
	float airResistanceConstant = 3; // Includes density and area
	float liftConstant = 160;

	glm::vec3 thrustForce = forward * (float)thrust * (thrust == 1 ? maxThrustForce : maxBreakForce);
	
	glm::vec3 airResitance = glm::vec3(0, 0, 0);
	if (glm::length(velocity) > 0) {
		float areaTopBottom = 1 + 10.3f * std::abs(glm::dot(up, glm::normalize(velocity)));
		float areaLeftRight = 1 + 1.2f * std::abs(glm::dot(glm::normalize(glm::cross(up, forward)), glm::normalize(velocity)));

		airResitance = -glm::normalize(velocity) * glm::length(velocity) * glm::length(velocity) * airResistanceConstant * areaTopBottom * areaLeftRight;
	}

	// Pitch
	float rotationSpeedPitch = 0.0099f * pitch * glm::length(velocity) / 100;
	glm::vec3 right = glm::cross(forward, up);
	forward = glm::normalize(glm::rotate(forward, rotationSpeedPitch, right));
	up = glm::normalize(glm::rotate(up, rotationSpeedPitch, right));
	
	// Roll
	float rotationSpeedRoll = -0.035f * roll * glm::length(velocity) / 90;
	up = glm::normalize(glm::rotate(up, rotationSpeedRoll, forward));

	glm::vec3 liftForce = up * dot(forward, velocity) * (liftConstant + pitch * 1);

	glm::vec3 gravity = glm::vec3(0, -mass * gravitationalAcceleration, 0);

	glm::vec3 netForce = gravity + thrustForce + airResitance + liftForce;
	glm::vec3 acceleration = netForce / mass;
	velocity = velocity + acceleration * dt;
	position = position + velocity * dt;

	std::cout << "Velocity: " << glm::length(velocity) << " m/s" << std::endl;
}