
uniform samplerCube cubeMap;

in vec3 fragmentVS;

out vec4 gl_Color;

void main() {
	vec4 ambient = vec4(0.8, 0.8, 0.8,1 );
	gl_Color = texture(cubeMap, fragmentVS) * ambient;
}
