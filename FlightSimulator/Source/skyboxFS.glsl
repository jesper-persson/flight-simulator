#version 460

uniform samplerCube cubeMap;

in vec3 fragmentVS;

out vec4 gl_Color;

void main() {
	gl_Color = texture(cubeMap, fragmentVS);
}
