#include "shader.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "Windows.h"

Shader::Shader(const char* vertexShaderSource, const char* fragmentShaderSource) {
	unsigned int vertexShader, fragmentShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);

	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	ID = glCreateProgram();
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	glLinkProgram(ID);

	int success;
	char infoLog[512];

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		MessageBoxA(NULL, infoLog, "Vertex shader compile error", MB_OK);
	}

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		MessageBoxA(NULL, infoLog, "Fragment shader compile error", MB_OK);
	}

	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		MessageBoxA(NULL, infoLog, "Shader link error", MB_OK);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	compute = false;
}

Shader::Shader(const char* computeShaderSource) {
	unsigned int computeShader;
	computeShader = glCreateShader(GL_COMPUTE_SHADER);

	glShaderSource(computeShader, 1, &computeShaderSource, nullptr);

	glCompileShader(computeShader);

	int success;
	char infoLog[512];

	glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
		MessageBoxA(NULL, infoLog, "Vertex shader compile error", MB_OK);
	}

	ID = glCreateProgram();
	glAttachShader(ID,computeShader);
	glLinkProgram(ID);

	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		MessageBoxA(NULL, infoLog, "Shader link error", MB_OK);
	}

	glDeleteShader(computeShader);

	compute = true;
}

void Shader::use() {
	glUseProgram(ID);
}

void Shader::passUniformInt(const char* name, int value) {
	glUniform1i(glGetUniformLocation(ID, name), value);
}

void Shader::passUniformFloat(const char* name, float value) {
	glUniform1f(glGetUniformLocation(ID, name), value);
}

void Shader::passUniformMat4(const char* name, glm::mat4 value) {
	glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(value));
}