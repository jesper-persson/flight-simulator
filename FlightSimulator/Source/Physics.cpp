#include "Physics.h"
#include <iostream>

#include <glm/gtx/transform.hpp> 
#include <glm/gtx/rotate_vector.hpp> 

void airplanePhysics(glm::vec3 &position, glm::vec3 &forward, glm::vec3 &up, glm::vec3 &velocity, int thrust, int roll, int pitch) {
	float dt = 0.016; // seconds

	glm::vec3 thrustForce = forward * (float)thrust * 0.7f;
	
	glm::vec3 airResitance = glm::vec3(0, 0, 0);
	if (glm::length(velocity) > 0) {
		airResitance = -glm::normalize(velocity) * glm::length(velocity) * glm::length(velocity) * 0.0006f;
	}

	glm::vec3 liftForce = up * dot(forward, velocity) * 0.013f;
	
	// Pitch
	float rotationSpeedPitch = 0.015f * pitch;
	glm::vec3 right = glm::cross(forward, up);
	forward = glm::normalize(glm::rotate(forward, rotationSpeedPitch, right));
	up = glm::normalize(glm::rotate(up, rotationSpeedPitch, right));
	
	// Roll
	float rotationSpeedRoll = -0.035f * roll;
	up = glm::normalize(glm::rotate(up, rotationSpeedRoll, forward));

	glm::vec3 gravity = glm::vec3(0, -0.3f, 0);

	glm::vec3 netForce = gravity + thrustForce + airResitance + liftForce;
	velocity = velocity + netForce;
	position = position + velocity * dt;
}