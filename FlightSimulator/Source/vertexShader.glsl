#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture;

uniform mat4 modelToWorld;
uniform mat4 projectionMatrix;
uniform mat4 worldToView;

out vec2 texture_out;
smooth out vec3 normal_out;
smooth out vec3 fragment_out;

void main(void) {
	normal_out = mat3(modelToWorld) * normal;
	texture_out = texture;
	fragment_out = vec3(modelToWorld * vec4(position, 1)).xyz;
	gl_Position = projectionMatrix * worldToView * modelToWorld * vec4(position, 1);
}