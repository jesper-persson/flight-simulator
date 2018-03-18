layout(location = 0) in vec3 positionModelSpace;
layout(location = 2) in vec2 textureTangentSpace;

uniform mat4 modelToWorld[100];
uniform mat4 parentTransformation;
uniform mat4 projectionMatrix;
uniform mat4 worldToView;
uniform float progress[100];

out vec4 colorVS;
out vec2 textureVS;
out float progressVS;

void main() {
	progressVS = progress[gl_InstanceID];
	textureVS = textureTangentSpace;

	mat4 viewcp = worldToView;
	gl_Position = projectionMatrix * viewcp * parentTransformation * modelToWorld[gl_InstanceID] * vec4(positionModelSpace, 1);
}