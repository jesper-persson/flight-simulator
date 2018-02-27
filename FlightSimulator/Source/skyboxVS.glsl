
layout(location = 0) in vec3 positionModelSpace;

uniform mat4 modelToWorld;
uniform mat4 worldToView;
uniform mat4 projectionMatrix;

out vec3 fragmentVS;

void main(void) {
	fragmentVS = vec3((modelToWorld * vec4(positionModelSpace, 1)).xyz);
	mat4 viewWithoutTranslation = worldToView;
	viewWithoutTranslation[3][0] = 0;
	viewWithoutTranslation[3][1] = 0;
	viewWithoutTranslation[3][2] = 0;
	gl_Position = projectionMatrix * viewWithoutTranslation * modelToWorld * vec4(positionModelSpace, 1);
}