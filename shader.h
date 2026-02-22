#pragma once
#include <glm/glm.hpp>
class Shader {
public:
	unsigned int ID;
	Shader() {};
	Shader(const char* vertexShaderSource, const char* fragmentShaderSource);
	Shader(const char* computeShaderSource);
	void use();
	void passUniformFloat(const char* name, float value);
	void passUniformMat4(const char* name, glm::mat4 value);
	void passUniformInt(const char* name, int value);
private:
	bool compute = false;
};