#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <ctime> 

#define WORLD_TO_INDEX(x, y, width) ((y) * (width) + (x))

int random(int min, int max) {
	return (rand() % (max - min)) + min;
}

// x and y is top left of square
template<typename T>
void diamondStep(int x, int y, int stepSize, int size, T *heightmap, float r) {
	float avg = (heightmap[WORLD_TO_INDEX(x, y, size)]
		+ heightmap[WORLD_TO_INDEX(x + stepSize, y, size)]
		+ heightmap[WORLD_TO_INDEX(x, y + stepSize, size)]
		+ heightmap[WORLD_TO_INDEX(x + stepSize, y + stepSize, size)]) / 4.0f;
	heightmap[WORLD_TO_INDEX(x + stepSize / 2, y + stepSize / 2, size)] = avg + r;
}

// x and y is middle of diamond
template<typename T>
void squareStep(int x, int y, int stepSize, int size, T *heightmap, float r) {
	float avg = 0;
	if (y - stepSize / 2 < 0 || y + stepSize / 2 >= size || x - stepSize / 2 < 0 || x + stepSize / 2 >= size) {
		float a = y - stepSize / 2 >= 0 ? heightmap[WORLD_TO_INDEX(x, y - stepSize / 2, size)] : 0;
		float b = y + stepSize / 2 < size ? heightmap[WORLD_TO_INDEX(x, y + stepSize / 2, size)] : 0;
		float c = x + stepSize / 2 < size ? heightmap[WORLD_TO_INDEX(x + stepSize / 2, y, size)] : 0;
		float d = x - stepSize / 2 >= 0 ? heightmap[WORLD_TO_INDEX(x - stepSize / 2, y, size)] : 0;
		avg = (a + b + c + d) / 3.0f;
	} else {
		float a = y - stepSize / 2 >= 0 ? heightmap[WORLD_TO_INDEX(x, y - stepSize / 2, size)] : 0;
		float b = y + stepSize / 2 < size ? heightmap[WORLD_TO_INDEX(x, y + stepSize / 2, size)] : 0;
		float c = x + stepSize / 2 < size ? heightmap[WORLD_TO_INDEX(x + stepSize / 2, y, size)] : 0;
		float d = x - stepSize / 2 >= 0 ? heightmap[WORLD_TO_INDEX(x - stepSize / 2, y, size)] : 0;
		avg = (a + b + c + d) / 4.0f;
	}
	heightmap[WORLD_TO_INDEX(x, y, size)] = avg + r;
}

// Heightmap must be of size 2^n + 1
// Smoothness
template<typename T>
T *diamondSquare(T *heightmap, const int size, float smoothness, time_t seed = -1) {
	
	assert((float)(int)(log2(size - 1)) == log2(size - 1));

	if (seed == -1) {
		seed = time(NULL);
	}
	srand(seed);

	// Pre seed corners
	heightmap[WORLD_TO_INDEX(0, 0, size)] = (random(0, 500) - 250) / smoothness;
	heightmap[WORLD_TO_INDEX(size - 1, 0, size)] = (random(0, 500) - 250) / smoothness;
	heightmap[WORLD_TO_INDEX(0, size - 1, size)] = (random(0, 500) - 250) / smoothness;
	heightmap[WORLD_TO_INDEX(size - 1, size - 1, size)] = (random(0, 500) - 250) / smoothness;

	int stepSize = size - 1;
	while (stepSize > 1) {
		smoothness = smoothness * 2;
		for (int y = 0; y < size - stepSize; y += stepSize) {
			for (int x = 0; x < size - stepSize; x += stepSize) {
				float r = (random(0, 500) - 250) / smoothness;
				diamondStep(x, y, stepSize, size, &heightmap[0], r);
			}
		}

		int i = 0;
		for (int y = 0; y < size; y += stepSize / 2) {
			i++;
			for (int x = (i % 2 == 0) ? 0 : stepSize / 2; x < size; x += stepSize) {
				float r = (random(0, 500) - 250) / smoothness;
				squareStep(x, y, stepSize, size, &heightmap[0], r);
			}
		}

		stepSize = stepSize / 2;
	}

	// Print heightmap
	//for (int y = 0; y < size; y += stepSize) {
	//	for (int x = 0; x < size; x += stepSize) {
	//		std::cout << heightmap[WORLD_TO_INDEX(x, y, size)] << "  ";
	//	}
	//	std::cout << std::endl;
	//}

	return heightmap;
}