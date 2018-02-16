#version 460

uniform sampler2D tex;

in vec2 texture_out;
in vec3 normal_out;
in vec3 fragment_out;

out vec4 gl_Color;

void main() {
	// Spotlight
	// vec3 lightPos = vec3(-2, 6, 0);
	// vec3 fragToLight = normalize(lightPos - fragment_out);
	// float diffuse = dot(fragToLight, normalize(normal_out));
	// if (diffuse < 0.3) {
	// 	   diffuse = 0.3;
	// }

	// Directional light
	vec3 lightDir = normalize(vec3(1.8, -2, 1));
	vec3 fragToLight = -lightDir;
	float diffuse = dot(fragToLight, normalize(normal_out)) * 1.5;
	if (diffuse < 0.3) {
		diffuse = 0.3;
	}

	gl_Color = texture(tex, texture_out * 3050) * diffuse;
	gl_Color.w = 1;
	//gl_Color = vec4(texture_out.xy,0,1);
}