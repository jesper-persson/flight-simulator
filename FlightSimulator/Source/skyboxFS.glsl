
uniform samplerCube cubeMap;
uniform samplerCube cubeMap2;
uniform float interpolation;

in vec3 fragmentVS;

out vec4 gl_Color;

void main() {
	vec4 ambient = vec4(1, 1, 1, 1);
	vec4 color1 = texture(cubeMap, fragmentVS);
	vec4 color2 = texture(cubeMap2, fragmentVS);
	gl_Color = mix(color2, color1, interpolation) * ambient;
}
