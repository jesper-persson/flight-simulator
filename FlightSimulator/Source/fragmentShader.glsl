#version 460

uniform sampler2D tex;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform samplerCube cubeMap;
uniform bool isTerrain;
uniform bool isSkybox;
uniform mat4 worldToView;

in vec2 texture_out;
smooth in vec3 normal_out;
smooth in vec3 fragment_out;

out vec4 gl_Color;

vec3 saturate(vec3 color, float saturate) {
    const vec3 w = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(color, w));
    return mix(intensity, color, saturate);
}

void main() {
	// Spotlight
	/*vec3 lightPos = vec3(5, 6, 5);
	vec3 fragToLight = normalize(lightPos - fragment_out);
	float diffuse = dot(fragToLight, normalize(normal_out));
	if (diffuse < 0.2) {
	 diffuse = 0.2;
	}*/

	// Directional light
	vec3 lightDir = normalize(vec3(1, -2, 1));
	vec3 fragToLight = -lightDir;
	float diffuse = dot(fragToLight, normalize(normal_out)) * 1.5;
	if (diffuse < 0.3) {
		diffuse = 0.3;
	}

	if (isTerrain) {
		vec4 terrain1 = texture(tex, texture_out);
		vec4 terrain2 = texture(tex2, texture_out);
		vec4 terrain3 = texture(tex3, texture_out);
		vec4 splat = texture(tex4, texture_out / 100);
		gl_Color = (splat.x * terrain1 + splat.y * terrain2 + splat.z * terrain3) * diffuse;
	}

	if (!isTerrain && !isSkybox) {
		gl_Color = texture(tex, texture_out) * diffuse;
	}

	mat4 invView = inverse(worldToView);
	vec3 cameraPos = vec3(invView[3][0], invView[3][1], invView[3][2]);

	if (isSkybox) {
		vec3 dirToFrag = normalize(fragment_out);
		gl_Color = texture(cubeMap, dirToFrag);
	}

	// Fade far away objects
	float camDistance = length(cameraPos - fragment_out);
	float distanceStartFade = 10000;
	float distanceEndFade = 60000;
	if (camDistance > distanceStartFade) {
		float saturation = clamp((camDistance - distanceStartFade) / (distanceEndFade - distanceStartFade), 0, 1);
		gl_Color = vec4(saturate(gl_Color.xyz, 1 - saturation), 1);
		gl_Color = mix(gl_Color, vec4(0.27, 0.43, 0.66, 1), saturation);
	}	

	gl_Color.w = 1;
}
