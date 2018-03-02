#include <glm\vec3.hpp>
#include "Common.h";

void steerAirplane(Entity &main, Entity &aileronLeft, Entity &aileronRight, Entity &leftFlap, Entity &rightFlap, int thrust, int roll, int pitch, float dt);

void airplanePhysics(Entity &entity, Entity &aileronLeft, Entity &leftFlap, float dt);

void normalPhysics(Entity &entity, float dt);