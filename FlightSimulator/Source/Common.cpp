#include "Common.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

std::string readFile(std::string path) {
	std::ifstream t(path);
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return str;
}