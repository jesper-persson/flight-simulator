#include <iostream>
#include <vector>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtx/euler_angles.hpp> 
#include <glm/gtx/transform.hpp> 
#include <glm/gtx/rotate_vector.hpp> 
#include <glm/gtx/quaternion.hpp>

#include "Rendering.h"
#include "ParticleSystem.h"
#include <lodepng.h>
#include "Common.h"
#include "DiamondSquare.h"
#include "Physics.h"
#include "EntityFactory.h"


float getHeightAt(float *heightmap, int size, float tileSize, float positionX, float positionZ) {
	int tileX = positionX / tileSize;
	int tileZ = positionZ / tileSize;

	// Find the four corners of the grid
	glm::vec3 topLeftCorner = glm::vec3(tileX * tileSize, heightmap[tileX + tileZ * size], tileZ * tileSize);
	glm::vec3 bottomRightCorner = glm::vec3((tileX + 1) * tileSize, heightmap[tileX + (tileZ + 1) * size + 1], (tileZ + 1) * tileSize);
	glm::vec3 topRightCorner = glm::vec3((tileX + 1) * tileSize, heightmap[tileX + tileZ * size + 1], tileZ * tileSize);
	glm::vec3 bottomLeftCorner = glm::vec3(tileX  * tileSize, heightmap[tileX + (tileZ + 1) * size], (tileZ + 1) * tileSize);

	// Calculate tile penetration
	float tilePenetrationX = ((int)(positionX * 100) % (int)(tileSize * 100)) / (tileSize * 100);
	float tilePenetrationZ = ((int)(positionZ * 100) % (int)(tileSize * 100)) / (tileSize * 100);

	float calculatedY;
	// Check if top left triangle
	if (tilePenetrationX <= 1 - tilePenetrationZ) {
		glm::vec3 one = topLeftCorner;
		glm::vec3 two = topRightCorner;
		glm::vec3 three = bottomLeftCorner;
		GLfloat lambda1N = ((two.z - three.z)*(positionX - three.x) + (three.x - two.x)*(positionZ - three.z));
		GLfloat lambda1D = ((two.z - three.z)*(one.x - three.x) + (three.x - two.x)*(one.z - three.z));
		GLfloat lambda1 = lambda1N / lambda1D;
		GLfloat lambda2N = ((three.z - one.z)*(positionX - three.x) + (one.x - three.x)*(positionZ - three.z));
		GLfloat lambda2D = ((two.z - three.z)*(one.x - three.x) + (three.x - two.x)*(one.z - three.z));
		GLfloat lambda2 = lambda2N / lambda2D;
		GLfloat lambda3 = 1 - lambda1 - lambda2;
		calculatedY = one.y * lambda1 + two.y * lambda2 + three.y * lambda3;
	} else {
		glm::vec3 one = bottomRightCorner;
		glm::vec3 two = topRightCorner;
		glm::vec3 three = bottomLeftCorner;
		GLfloat lambda1N = ((two.z - three.z)*(positionX - three.x) + (three.x - two.x)*(positionZ - three.z));
		GLfloat lambda1D = ((two.z - three.z)*(one.x - three.x) + (three.x - two.x)*(one.z - three.z));
		GLfloat lambda1 = lambda1N / lambda1D;
		GLfloat lambda2N = ((three.z - one.z)*(positionX - three.x) + (one.x - three.x)*(positionZ - three.z));
		GLfloat lambda2D = ((two.z - three.z)*(one.x - three.x) + (three.x - two.x)*(one.z - three.z));
		GLfloat lambda2 = lambda2N / lambda2D;
		GLfloat lambda3 = 1 - lambda1 - lambda2;
		calculatedY = one.y * lambda1 + two.y * lambda2 + three.y * lambda3;
	}

	return calculatedY;
}

// Should also interpolate over triangle
void terrainCollision(float *heightmap, int size, float tileSize, Entity &entity) {
	float height = getHeightAt(heightmap, size, tileSize, entity.position.x, entity.position.z);
	if (height > entity.position.y + entity.centerToGroundContactPoint) {
		entity.position.y = height - entity.centerToGroundContactPoint;
		entity.velocity.y = 0;
	}
}

int steerMode = 0;
glm::vec3 cameraPosition = glm::vec3(10, 10, 10);
glm::vec3 cameraForward = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
Entity *entityToFollow = nullptr;
bool isForward, isBackward, isLeft, isUp, isRight, isDown, isStrideLeft, isStrideRight, jump, isShift;

void basicSteering(glm::vec3 &position, glm::vec3 &forward, glm::vec3 &up) {
	GLfloat rotationSpeed = 0.03f;
	GLfloat speed = 0.09f * (isShift ? 100 : 1);
	if (isForward)
	{
		position = position + forward * speed;
	}
	if (isBackward)
	{
		position = position - forward * speed;
	}
	if (isStrideLeft)
	{
		glm::vec3 left = glm::normalize(glm::cross(up, forward));
		position = position + left * speed;
	}
	if (isStrideRight)
	{
		glm::vec3 right = glm::normalize(glm::cross(forward, up));
		position = position + right * speed;
	}
	if (isLeft)
	{
		forward = glm::normalize(glm::rotate(forward, rotationSpeed, glm::vec3(0, 1, 0)));
		up = glm::normalize(glm::rotate(up, rotationSpeed, glm::vec3(0, 1, 0)));
	}
	if (isRight)
	{
		forward = glm::normalize(glm::rotate(forward, -rotationSpeed, glm::vec3(0, 1, 0)));
		up = glm::normalize(glm::rotate(up, -rotationSpeed, glm::vec3(0, 1, 0)));
	}
	if (isUp)
	{
		glm::vec3 right = glm::cross(forward, up);
		forward = glm::normalize(glm::rotate(forward, -rotationSpeed, right));
		up = glm::normalize(glm::rotate(up, -rotationSpeed, right));
	}
	if (isDown)
	{
		glm::vec3 right = glm::cross(forward, up);
		forward = glm::normalize(glm::rotate(forward, rotationSpeed, right));
		up = glm::normalize(glm::rotate(up, rotationSpeed, right));
	}
}

void printVector(glm::vec3 &v) {
	std::cout << v.x << ", " << v.y << ", " << v.z << std::endl;
}

void handleKeyChange(bool* currentValue, int action) {
	if (action == GLFW_PRESS) {
		*currentValue = true;
	} else if (action == GLFW_RELEASE) {
		*currentValue = false;
	}
}

void interpolateCamera(glm::vec3 &targetPosition, glm::vec3 &cameraPosition, float dt) {
	glm::vec3 direction = glm::normalize(targetPosition - cameraPosition);
	if (glm::length(direction) > 0.5f) {
		cameraPosition = cameraPosition + direction * dt * glm::length(targetPosition - cameraPosition) / 1.0f * 16.0f;
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		steerMode++;
		if (steerMode == 3) {
			steerMode = 0;
		}
	}
	
	if (key == GLFW_KEY_LEFT_SHIFT) {
		handleKeyChange(&isShift, action);
	}
	
	if (key == GLFW_KEY_W) {
		handleKeyChange(&isForward, action);
	}

	if (key == GLFW_KEY_S) {
		handleKeyChange(&isBackward, action);
	}

	if (key == GLFW_KEY_A) {
		handleKeyChange(&isStrideLeft, action);
	}

	if (key == GLFW_KEY_D) {
		handleKeyChange(&isStrideRight, action);
	}

	if (key == GLFW_KEY_LEFT) {
		handleKeyChange(&isLeft, action);
	}

	if (key == GLFW_KEY_RIGHT) {
		handleKeyChange(&isRight, action);
	}

	if (key == GLFW_KEY_UP) {
		handleKeyChange(&isUp, action);
	}

	if (key == GLFW_KEY_DOWN) {
		handleKeyChange(&isDown, action);
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		jump = true;
	}
}

static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

void makeRunwayOnHeightmap(float *heightmap, int size) {
	float avgHeight = 280;
	for (int x = 8; x <= 12; x++) {
		for (int z = 8; z <= 80; z++) {
			heightmap[z * size + x] = avgHeight;
		}
	}
}

void log(std::string output) {
	std::cout << output << std::endl;
}

void glDebugMessageCallbackFunction(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
	if (type == GL_DEBUG_TYPE_MARKER) {
		return;
	}
	switch (source) {
	case GL_DEBUG_SOURCE_API:
		log("Source: GL_DEBUG_SOURCE_API");
		break;

	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		log("Source: GL_DEBUG_SOURCE_WINDOW_SYSTEM");
		break;

	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		log("Source: GL_DEBUG_SOURCE_SHADER_COMPILER");
		break;

	case GL_DEBUG_SOURCE_THIRD_PARTY:
		log("Source: GL_DEBUG_SOURCE_THIRD_PARTY");
		break;

	case GL_DEBUG_SOURCE_APPLICATION:
		log("Source: GL_DEBUG_SOURCE_APPLICATION");
		break;

	case GL_DEBUG_SOURCE_OTHER:
		log("Source: GL_DEBUG_SOURCE_OTHER");
		break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		log("Type: GL_DEBUG_TYPE_ERROR");
		break;

	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		log("Type: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR");
		break;

	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		log("Type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR");
		break;

	case GL_DEBUG_TYPE_PORTABILITY:
		log("Type: GL_DEBUG_TYPE_PORTABILITY");
		break;

	case GL_DEBUG_TYPE_PERFORMANCE:
		log("Type: GL_DEBUG_TYPE_PERFORMANCE");
		break;

	case GL_DEBUG_TYPE_MARKER:
		log("Type: GL_DEBUG_TYPE_MARKER");
		break;

	case GL_DEBUG_TYPE_PUSH_GROUP:
		log("Type: GL_DEBUG_TYPE_PUSH_GROUP");
		break;

	case GL_DEBUG_TYPE_POP_GROUP:
		log("Type: GL_DEBUG_TYPE_POP_GROUP");
		break;

	case GL_DEBUG_TYPE_OTHER:
		log("Type: GL_DEBUG_TYPE_OTHER");
		break;
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		log("Severity: GL_DEBUG_SEVERITY_HIGH");
		break;

	case GL_DEBUG_SEVERITY_MEDIUM:
		log("Severity: GL_DEBUG_SEVERITY_MEDIUM");
		break;

	case GL_DEBUG_SEVERITY_LOW:
		log("Severity: GL_DEBUG_SEVERITY_LOW");
		break;

	case GL_DEBUG_SEVERITY_NOTIFICATION:
		log("Severity: GL_DEBUG_SEVERITY_NOTIFICATION");
		break;
	}

	log(id);
	log(message);
}

int program();

int main() {
	program();
}

int program() {
	const int windowHeight = 900;
	const int windowWidth = 1700;
	glm::mat4 perspective = glm::perspective<GLfloat>(0.8f, windowWidth / (float)windowHeight, .1f, 1500);
	glm::mat4 perspectiveForSun = glm::perspective<GLfloat>(0.8f, windowWidth / (float)windowHeight, .1f, 51500);

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Flight Simulator", NULL, NULL);
	if (window == NULL) {
		std::cerr << "Error! Failed to create Window!" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Error! Failed to initlize OpenGL" << std::endl;
	}

	GLuint ids[] = { 0x20071 };
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 1, ids, GL_FALSE);
	glDebugMessageCallback(glDebugMessageCallbackFunction, 0);

	GLuint modelShader = getShader("Source/modelVS.glsl", "Source/modelFS.glsl");
	GLuint skyboxShader = getShader("Source/skyboxVS.glsl", "Source/skyboxFS.glsl");
	GLuint terrainShader = getShader("Source/terrainVS.glsl", "Source/terrainFS.glsl");
	GLuint instancedShader = getShader("Source/instancedVS.glsl", "Source/instancedFS.glsl");
	int uniformLocationInstanceShaderTransformation[NUM_PARTICLES_PER_DRAW_CALL];
	int uniformLocationInstanceShaderProgress[NUM_PARTICLES_PER_DRAW_CALL];
	for (int i = 0; i < NUM_PARTICLES_PER_DRAW_CALL; i++) {
		std::string uniformName = std::string("modelToWorld[") + std::to_string(i) + std::string("]");
		std::string uniformNameColor = std::string("progress[") + std::to_string(i) + std::string("]");
		uniformLocationInstanceShaderTransformation[i] = glGetUniformLocation(instancedShader, uniformName.c_str());
		uniformLocationInstanceShaderProgress[i] = glGetUniformLocation(instancedShader, uniformNameColor.c_str());
	}

	int size = 2049;
	int tileSizeXZ = 2;
	int textureScale = 500; // Must also change terrain fragment shader
	int tileSizeY = 1;
	float smothness = 0.3f;
	if (FAST_MODE) {
		//size = 129;
		//tileSizeXZ = 10;
		//smothness = 1;
	}

	time_t seed = 1519128009; // Splat map is built after this seed, so don't change it
	float *heightmapData = new float[size * size];
	diamondSquare(heightmapData, size, smothness, seed);
	makeRunwayOnHeightmap(heightmapData, size);

	Model terrain = heightmapToModel(heightmapData, size, size, tileSizeXZ, tileSizeY, tileSizeXZ,  textureScale);

	Terrain ground = Terrain();
	ground.setModel(terrain);
	ground.position = glm::vec3(0, 0, 0);
	ground.scale = glm::vec3(1, 1, 1);
	ground.textureId = loadPNGTexture("Resources/grass512.png");
	ground.setTextureId2(loadPNGTexture("Resources/asphalt512.png"));
	ground.setTextureId3(loadPNGTexture("Resources/sand512.png"));
	ground.setTextureId4(loadPNGTexture("Resources/terrain-splatmap.png"));

	GLuint jasTexture = loadPNGTexture("Resources/jas.png");
	GLuint jasNormalMap = loadPNGTexture("Resources/normalmap.png");
	std::vector<Entity*> airplane = loadJAS39Gripen("Resources/jas.obj");
	airplane[0]->position = glm::vec3(20, 10, 20);
	airplane[0]->scale = glm::vec3(0.2f, 0.2f, 0.2f);
	airplane[0]->centerToGroundContactPoint = -0.2f;
	for (std::vector<Entity*>::iterator iter = airplane.begin(); iter != airplane.end(); iter++) {
		(*iter)->textureId = jasTexture;
		(*iter)->normalMapId = jasNormalMap;
	}

	Entity skybox = Entity();
	skybox.setModel(getVAOCube());
	skybox.scale = glm::vec3(20, 20, 20);
	skybox.position = glm::vec3(0, -10, 0);
	GLuint secondSkybox;
	if (FAST_MODE) {
		skybox.textureId = createSkybox("Resources/skybox-x-.png", "Resources/skybox-x+.png", "Resources/skybox-y+.png", "Resources/skybox-y-.png", "Resources/skybox-z-.png", "Resources/skybox-z+.png");
		secondSkybox = skybox.textureId;
	} else {
		skybox.textureId = createSkybox("Resources/skybox-x-.png", "Resources/skybox-x+.png", "Resources/skybox-y+.png", "Resources/skybox-y-.png", "Resources/skybox-z-.png", "Resources/skybox-z+.png");
		secondSkybox = createSkybox("Resources/skybox-night-x-.png", "Resources/skybox-night-x+.png", "Resources/skybox-night-y+.png", "Resources/skybox-night-y-.png", "Resources/skybox-night-z-.png", "Resources/skybox-night-z+.png");
	}

	Entity cube = Entity();
	cube.setModel(getVAOCube());
	cube.scale = glm::vec3(0.1, 0.1, 0.1);
	cube.position = glm::vec3(0, 0, 0);
	cube.textureId = loadPNGTexture("Resources/grass512.png");
	cube.normalMapId = loadPNGTexture("Resources/normalmap.png");

	Entity player = Entity();
	player.setModel(getVAOCube());
	player.scale = glm::vec3(0.1, 0.4, 0.1);
	player.position = glm::vec3(10, 100, 10);
	player.getModel().color = glm::vec4(0.7f, 0.84f, 0.54f, 1.0f);
	player.centerToGroundContactPoint = -0.4f;

	std::vector<Light*> lights;

	Light playerLight = createPointLight(glm::vec3(0, 1.2, 0), glm::vec3(0.05, 0.05, 0.05), 3, 0.1, 0.1, glm::vec3(0.3, 1, 0.8));
	playerLight.setParentEntity(&player);
	lights.push_back(&playerLight);

	Light sun = createDirectionalLight(glm::vec3(50, 50, 50), glm::vec3(100, 100, 100), 13.4, glm::vec3(0, -1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 1, 1));
	lights.push_back(&sun);

	Light airplaneSpotlight = createSpotlight(glm::vec3(0, 0, 10), glm::vec3(0.1, 0.1, 0.1), 10, 0.00001, 0.1, 0.2f, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec3(1, 1, 1));
	airplaneSpotlight.setParentEntity(airplane[0]);
	lights.push_back(&airplaneSpotlight);

	Light airplaneWingLight = createPointLight(glm::vec3(-4, 0.1, -3), glm::vec3(0.05, 0.05, 0.05), 3, 0.1, 0.1, glm::vec3(1, 0.2, 0.2));
	airplaneWingLight.setParentEntity(airplane[0]);
	lights.push_back(&airplaneWingLight);

	Light airplaneWingLightLeft = createPointLight(glm::vec3(4, 0.1, -3), glm::vec3(0.05, 0.05, 0.05), 3, 0.1, 0.1, glm::vec3(1, 0.2, 0.2));
	airplaneWingLightLeft.setParentEntity(airplane[0]);
	lights.push_back(&airplaneWingLightLeft);

	Light worldGreenMovingLight = createPointLight(glm::vec3(100, 50, 100), glm::vec3(1, 1, 1), 10, 0.001, 0.001, glm::vec3(0, 1, 0.55));
	worldGreenMovingLight.centerToGroundContactPoint = -10;
	lights.push_back(&worldGreenMovingLight);

	// Particles
	Model particleModel = getVAOQuad();
	ParticleSystem smoke = ParticleSystem(300 * 2);
	smoke.model = particleModel;
	smoke.particlesPerSecond = 300;
	smoke.timeSinceLastSpawn = 0;
	smoke.minLifetime = 0.9f;
	smoke.maxLifetime = 2.0f;
	smoke.minSize = 0.2f;
	smoke.maxSize = 0.4f;
	smoke.sphereRadiusSpawn = 1.0f;
	smoke.textureId = loadPNGTexture("Resources/particle-atlas2.png");
	smoke.atlasSize = 8;
	smoke.velocity = 3.25f;
	smoke.position = glm::vec3(0, 0.25f, -5.9f);
	smoke.setDirection(-0.08, 0.08, -0.08, 0.08, -1.1, -1);
	smoke.parentEntity = airplane[0];
	smoke.followParent = true;

	// Wingtip
	ParticleSystem wingtip = ParticleSystem(20000);
	wingtip.model = particleModel;
	wingtip.particlesPerSecond = 2500;
	wingtip.timeSinceLastSpawn = 0;
	wingtip.minLifetime = 0.1f;
	wingtip.maxLifetime = 2.4f;
	wingtip.minSize = .03f;
	wingtip.maxSize = .1f;
	wingtip.sphereRadiusSpawn = 0.2f;
	wingtip.textureId = loadPNGTexture("Resources/particle-atlas3.png");
	wingtip.atlasSize = 8;
	wingtip.velocity = 0.0000001f;
	wingtip.position = glm::vec3(4.0f, 0.0f, -4.2f);
	wingtip.setDirection(-0.1f, 0.1f, -0.1f, 0.1f, -200, -100);
	wingtip.parentEntity = airplane[0];
	wingtip.followParent = false;
	// Wingtip2
	ParticleSystem wingtip2 = ParticleSystem(20000);
	wingtip2.model = particleModel;
	wingtip2.particlesPerSecond = 2500;
	wingtip2.timeSinceLastSpawn = 0;
	wingtip2.minLifetime = 0.1f;
	wingtip2.maxLifetime = 2.4f;
	wingtip2.minSize = .03f;
	wingtip2.maxSize = .1f;
	wingtip2.sphereRadiusSpawn = 0.2f;
	wingtip2.textureId = loadPNGTexture("Resources/particle-atlas3.png");
	wingtip2.atlasSize = 8;
	wingtip2.velocity = 0.0000001f;
	wingtip2.position = glm::vec3(-4.0f, 0.0f, -4.2f);
	wingtip2.setDirection(-0.1f, 0.1f, -0.1f, 0.1f, -200, -100);
	wingtip2.parentEntity = airplane[0];
	wingtip2.followParent = false;


	glClearColor(1, 0.43, 0.66, 0.0f);
	glEnable(GL_DEPTH_TEST);

	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);
	//glCullFace(GL_BACK);

	bool boom = false;
	float lastTime = glfwGetTime();
	float currentTime;
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float currentTime = glfwGetTime();
		float dt = currentTime - lastTime;
		lastTime = currentTime;
		std::string title = std::string("Frame time: ") + std::to_string(dt) + std::string(" micro seconds");
		glfwSetWindowTitle(window, title.c_str());

		if (steerMode == 0) { // Camera
			basicSteering(cameraPosition, cameraForward, cameraUp);
			entityToFollow = nullptr;
		} else if (steerMode == 1) { // Airplane
			if (!boom) {
				steerAirplane(*airplane[0], *airplane[18], *airplane[1], *airplane[2], *airplane[14], isForward ? 1 : (isBackward ? -1 : 0), isLeft ? 1 : (isRight ? -1 : 0), isDown ? 1 : (isUp ? -1 : 0), dt);
				entityToFollow = airplane[0];
			}
		} else if (steerMode == 2) { // Player
			entityToFollow = &player;
			basicSteering(player.position, player.forward, player.up);
		}

		bool collision = getHeightAt(heightmapData, size, tileSizeXZ, airplane[0]->position.x, airplane[0]->position.z) >= airplane[0]->position.y - 0.2f;

		airplanePhysics(*airplane[0], *airplane[18], *airplane[2], dt);	
		terrainCollision(heightmapData, size, tileSizeXZ, *airplane[0]);
		runPhysics(player, dt);
		terrainCollision(heightmapData, size, tileSizeXZ, player);

		for (int i = 0; i < lights.size(); i++) {
			if (lights[i]->getParentEntity()) {
				continue;
			}
			runPhysics(*lights[i], dt);
			terrainCollision(heightmapData, size, tileSizeXZ, *lights[i]);
		}

		// Camera
		glm::mat4 cam = glm::lookAt(cameraPosition, cameraPosition + cameraForward * 14.0f, cameraUp);
		if (entityToFollow) {
			glm::vec3 targetPosition = entityToFollow->position + -entityToFollow->forward * 2.5f * (boom ? 2.0f : 1.0f) + entityToFollow->up * 1.0f  * (boom ? 2.0f : 1.0f);
			//cameraPosition = targetPosition;
			interpolateCamera(targetPosition, cameraPosition, dt);
			float terrainHeightAtCamera = getHeightAt(heightmapData, size, tileSizeXZ, cameraPosition.x, cameraPosition.z);
			if (cameraPosition.y <= terrainHeightAtCamera + 0.05f) {
				cameraPosition.y = terrainHeightAtCamera + 0.05f;
			}
			cameraForward = glm::normalize(entityToFollow->position - cameraPosition);
			cam = glm::lookAt(cameraPosition, entityToFollow->position, entityToFollow->up);
			cameraUp = entityToFollow->up;
		}

		worldGreenMovingLight.position.x = 100 * sin(glfwGetTime() / (float)10) + 400;
		worldGreenMovingLight.position.z = 100 * cos(glfwGetTime() / (float)10) + 400;
		worldGreenMovingLight.intensity = 4 * sin(glfwGetTime() * 2) + 4;

		// Airplane lights
		airplaneWingLight.intensity = 4;
		airplaneWingLightLeft.intensity = 4;
		if ((int)(glfwGetTime() * 10) % 10 != 0) {
			airplaneWingLight.intensity = 0;
			airplaneWingLightLeft.intensity = 0;
		}

		// Animate sun (make sure direction always faces middle of ground
		float centerPosition = (float)(size * tileSizeXZ) / 2.0f;
		glm::vec3 center = glm::vec3(centerPosition, getHeightAt(heightmapData, size, tileSizeXZ, centerPosition, centerPosition), centerPosition);
		float time = glfwGetTime() / 8 + 10;
		sun.position.z = centerPosition;
		sun.position.y = cos(time) * centerPosition * 2 + center.y;
		sun.position.x = sin(time) * centerPosition * 2 + centerPosition;
		sun.up = glm::vec3(0, 0, 1);
		sun.forward = glm::normalize(center - sun.position);
		float interpolation = (cos(time) + 1) / 2.0f;

		// Render entities
		renderSkybox(skybox, secondSkybox, interpolation, skyboxShader, cam, perspective);
		bindLight(terrainShader, lights);
		bindLight(modelShader, lights);
		renderTerrain(ground, terrainShader, cam, perspective);
		for (int i = 0; i < airplane.size(); i++) {
			if (i == 8 || i == 0) {
				continue;
			}
			renderEntity(*airplane[i], modelShader, cam, perspective, true);
		}
		renderEntity(player, modelShader, cam, perspective, true);
		renderEntity(*airplane[0], modelShader, cam, perspective, true);
		renderEntity(*airplane[8], modelShader, cam, perspective, true);
		renderEntity(cube, modelShader, cam, perspective, true);

		// Particles
		Entity parentEntity = *airplane[0];
		updateParticleSystem(smoke, cameraPosition, cameraForward, dt);
		updateParticleSystem(wingtip, cameraPosition, cameraForward, dt);
		updateParticleSystem(wingtip2, cameraPosition, cameraForward, dt);
		if (smoke.numParticles > 0) {
			renderParticleSystem(smoke, instancedShader, uniformLocationInstanceShaderTransformation, uniformLocationInstanceShaderProgress, cam, perspective);
		}
		if (wingtip.numParticles > 0) {
			renderParticleSystem(wingtip, instancedShader, uniformLocationInstanceShaderTransformation, uniformLocationInstanceShaderProgress, cam, perspective);
		}
		if (wingtip2.numParticles > 0) {
			renderParticleSystem(wingtip2, instancedShader, uniformLocationInstanceShaderTransformation, uniformLocationInstanceShaderProgress, cam, perspective);
		}

		// Draw lights
		for (int i = 0; i < lights.size(); i++) {
			//renderEntity(*lights[i], modelShader, cam, perspectiveForSun, false);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete heightmapData;
	
	for (std::vector<Entity*>::iterator iter = airplane.begin(); iter != airplane.end(); iter++) {
		delete *iter;
	}

	return 0;
}