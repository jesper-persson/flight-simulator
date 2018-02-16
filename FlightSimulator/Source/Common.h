#include <string>
#include <glad\glad.h>

std::string readFile(std::string path);

struct Model {
public:
	GLuint vao;
	int numIndices;
};

Model objLoader(std::string path);