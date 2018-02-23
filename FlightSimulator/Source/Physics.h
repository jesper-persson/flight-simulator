#include <glm\vec3.hpp>
#include "Common.h";

void steerAirplane(Entity &main, Entity &aileronLeft, Entity &aileronRight, Entity &leftFlap, Entity &rightFlap, int thrust, int roll, int pitch, float dt);

void airplanePhysics(Entity &entity, glm::vec3 &position, glm::vec3 &forward, glm::vec3 &up, glm::vec3 &velocity, int thrust, int roll, int pitch, float dt);