#version 460

layout(location = 0) in vec3 positionModelSpace;
layout(location = 1) in vec3 normalModelSpace;
layout(location = 2) in vec2 textureTangentSpace;

uniform mat4 modelToWorld;
uniform mat4 projectionMatrix;
uniform mat4 worldToView;

out vec3 lightDirectionVS;
out vec3 viewPositionVS;
out vec2 textureVS;
out vec3 normalVS;
out vec3 fragmentVS;

void main(void) {
	normalVS = normalize(mat3(modelToWorld) * normalModelSpace);

	lightDirectionVS = normalize(vec3(1, -1, -1));
	mat4 invView = inverse(worldToView);
	viewPositionVS = vec3(invView[3][0], invView[3][1], invView[3][2]);

	textureVS = textureTangentSpace;
	fragmentVS = vec3((modelToWorld * vec4(positionModelSpace, 1)).xyz);
	gl_Position = projectionMatrix * worldToView * modelToWorld * vec4(positionModelSpace, 1);
}