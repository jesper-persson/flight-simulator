#include "Physics.h"
#include <iostream>
#include <algorithm>

#include <glm/gtx/transform.hpp> 
#include <glm/gtx/rotate_vector.hpp> 

// dt in seconds
// assumes 1 distance unit in the game is 1 meter
void airplanePhysics(glm::vec3 &position, glm::vec3 &forward, glm::vec3 &up, glm::vec3 &velocity, int thrust, int roll, int pitch, float dt) {
	float mass = 1000;
	float gravitationalAcceleration = 9.82f;
	float maxThrustForce = 29000; // Newtons
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