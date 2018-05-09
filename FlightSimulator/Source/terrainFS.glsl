uniform sampler2D tex;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform Light lights[10]; // Max 10 ligths

in vec3 viewPositionVS;
in vec2 textureVS;
in vec3 normalVS;
in vec3 fragmentVS;

out vec4 gl_Color;

vec3 saturate(vec3 color, float saturate) {
    const vec3 w = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(color, w));
    return mix(intensity, color, saturate);
}

void main() {
	vec4 terrain1 = texture(tex, textureVS);
	vec4 terrain2 = texture(tex2, textureVS);
	vec4 terrain3 = texture(tex3, textureVS);
	vec4 splat = texture(tex4, textureVS / 500);
	vec4 terrainColor = (splat.x * terrain1 + splat.y * terrain2 + splat.z * terrain3);


	// Combine lights
	vec4 totalLight = vec4(0, 0, 0, 0);
	for (int i = 0; i < 10; i++) {
		totalLight = totalLight + calculateLight(lights[i], vec3(0.5, 0.5, 0.5), vec3(0, 0, 0), fragmentVS, normalVS, viewPositionVS);
	}
	gl_Color = terrainColor * totalLight;

	// Fade far away objects
	float camDistance = length(viewPositionVS - fragmentVS);
	float distanceStartFade = 100;
	float distanceEndFade = 1000;
	if (camDistance > distanceStartFade) {
		float saturation = clamp((camDistance - distanceStartFade) / (distanceEndFade - distanceStartFade), 0, 1);
		gl_Color = vec4(saturate(gl_Color.xyz, 1 - saturation), 1);
		vec4 fadeTargetDay = vec4(0.27, 0.43, 0.66, 1);
		vec4 fadeTargetNight = vec4(72/255f, 93/255f, 144/255f, 1);
		vec4 fadeTarget =  vec4(1, 1, 1, 1) * fadeTargetNight;
		gl_Color = mix(gl_Color, fadeTarget, saturation);
	}
	
	//gl_Color.w = 1;
}
