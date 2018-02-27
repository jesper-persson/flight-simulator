
uniform sampler2D tex;
uniform sampler2D normalMap;
uniform mat4 worldToView;

uniform bool useLights;

uniform vec3 Ka, Kd, Ks;
uniform float specularExponent;
uniform float dissolve;
uniform vec4 color;

in vec3 fragmentPositionTangentSpaceVS;
in vec3 viewPositionTangentSpaceVS;
in vec3 lightDirectionTangentSpaceVS;
in vec3 lightDirectionVS;
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
	vec4 ambient = vec4(0.5,0.5,0.5,1);

	vec3 normal = normalize(normalVS);
	float normalMapScale = 40;
	vec3 normalTangentSpace = normalize(vec3(texture(normalMap, textureVS * normalMapScale).xyz) * 2 - 1); 
	vec3 lightDirection = lightDirectionVS;
	vec3 fragment = fragmentVS;
	vec3 viewPosition = viewPositionVS;

	bool useNormalMap = false;
	if (useNormalMap) {
		normal = normalTangentSpace;
		lightDirection = lightDirectionTangentSpaceVS;
		viewPosition = viewPositionTangentSpaceVS;
		fragment = fragmentPositionTangentSpaceVS;
	}

	// Directional light
	vec3 fragToLight = -lightDirection;
	float diffuse = dot(fragToLight, normal) * 1.5;
	vec4 diffuseColor = vec4(Kd, 1);
	

	// Specular highlight
	vec3 reflectionDirection = reflect(lightDirection, normalize(normal));
	vec3 surfaceToCamera = normalize(viewPosition - fragment);
	float cosAngle = max(0.0, dot(surfaceToCamera, reflectionDirection));
	float specularCoefficient = pow(cosAngle, specularExponent);
	vec4 specularColor = vec4(Ks.xyz, 1);

	//  Fix issue where specular shines through object (not really)
	if (dot(normal, lightDirection) >= 0) {
		specularCoefficient = 0;
	}


	vec4 totalLight = vec4(1, 1, 1, 1);
	if (useLights) {
		totalLight = (ambient + diffuseColor * diffuse + specularColor * specularCoefficient);
	}

	vec4 textureColor = texture(tex, textureVS) + color;
	gl_Color = textureColor * totalLight;


	// Fade far away objects
	// float camDistance = length(viewPositionVS - fragmentVS);
	// float distanceStartFade = 100;
	// float distanceEndFade = 1000;
	// if (camDistance > distanceStartFade) {
	//	float saturation = clamp((camDistance - distanceStartFade) / (distanceEndFade - distanceStartFade), 0, 1);
	//	gl_Color = vec4(saturate(gl_Color.xyz, 1 - saturation), 1);
	//	gl_Color = mix(gl_Color, vec4(0.27, 0.43, 0.66, 1), saturation);
	// }
	
	gl_Color.w = dissolve;
}
