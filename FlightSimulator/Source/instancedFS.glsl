out vec4 gl_Color;

in vec4 colorVS;
in vec2 textureVS;
in float progressVS;

uniform int atlasSize;
uniform sampler2D tex;

vec4 getSprite(int index) {
	vec2 offset = vec2(float(index%atlasSize)/atlasSize, (atlasSize - 1 - index/atlasSize) * 1.0f/atlasSize);
	vec2 scale = vec2(1.0f/atlasSize, 1.0f/atlasSize);
	return texture(tex, textureVS * scale + offset);
}

void main() {
	//gl_Color = vec4(colorVS);

	float progressPerImage = 1.0f / (atlasSize * atlasSize);

	int i = int(progressVS * atlasSize * atlasSize);
	float progressNeededForNextSprite = float(i + 1) / float(atlasSize * atlasSize);

	float interp = (progressNeededForNextSprite - progressVS) / float(progressPerImage);
	if (interp>=1) {
	interp = 1;
	}
	if (interp<=0) {
	interp =0;}

	vec4 sprite1 = getSprite(i);
	vec4 sprite2 = getSprite(i + 1);
	if (i == atlasSize * atlasSize - 1) {
		interp = 1;
	}
	vec4 color = mix(sprite2, sprite1, interp);
	gl_Color = color * vec4(1.3,1,1.3,1);
}

