#version 460

layout(location = 0) in vec3 positionModelSpace;
layout(location = 1) in vec3 normalModelSpace;
layout(location = 2) in vec2 textureTangentSpace;
layout(location = 3) in vec3 tangentModelSpace;
layout(location = 4) in vec3 bitangentModelSpace;

uniform mat4 modelToWorld;
uniform mat4 projectionMatrix;
uniform mat4 worldToView;
uniform bool isSkybox;

out vec3 viewPositionTangentSpaceVS;
out vec3 lightDirectionTangentSpaceVS;
out vec3 fragmentPositionTangentSpaceVS;
out vec3 lightDirectionVS;
out vec3 viewPositionVS;
out vec2 textureVS;
out vec3 normalVS;
out vec3 fragmentVS;

void main(void) {
	normalVS = normalize(mat3(modelToWorld) * normalModelSpace);
	vec3 tangent = normalize(mat3(modelToWorld) * tangentModelSpace);
	vec3 bitangent = normalize(mat3(modelToWorld) * bitangentModelSpace);

	mat3 TBN = transpose(mat3(tangent, bitangent, normalVS));

	lightDirectionVS = normalize(vec3(1, -1, -1));
	lightDirectionTangentSpaceVS = TBN * lightDirectionVS;
	mat4 invView = inverse(worldToView);
	viewPositionVS = vec3(invView[3][0], invView[3][1], invView[3][2]);
	viewPositionTangentSpaceVS = TBN * viewPositionVS;

	textureVS = textureTangentSpace;
	fragmentVS = vec3((modelToWorld * vec4(positionModelSpace, 1)).xyz);
	fragmentPositionTangentSpaceVS = TBN * fragmentVS;
	gl_Position = projectionMatrix * worldToView * modelToWorld * vec4(positionModelSpace, 1);

	if (isSkybox) {
		mat4 viewWithoutTranslation = worldToView;
		viewWithoutTranslation[3][0] = 0;
		viewWithoutTranslation[3][1] = 0;
		viewWithoutTranslation[3][2] = 0;
		gl_Position = projectionMatrix * viewWithoutTranslation * modelToWorld * vec4(positionModelSpace, 1);
	}
}