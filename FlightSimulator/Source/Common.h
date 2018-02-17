#include <string>
#include <glad\glad.h>

std::string readFile(std::string path);

struct Model {
public:
	GLuint vao;
	int numIndices;
};

Model heightmapToModel(float *heightmap, int width, int height, float scaleX, float scaleY, float scaleZ, float textureScale);

Model modelFromVertexData(float vertexCoordinates[], int vertexCoordinatesSize, float normals[], int normalsSize, float textureCoordinates[], int textureCoordinatesSize, int indices[], int indicesSize);