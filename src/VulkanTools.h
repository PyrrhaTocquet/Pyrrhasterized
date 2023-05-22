#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

namespace vkTools {
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary); //Start reading at the end of the file, Read the file as a binary file

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}



}