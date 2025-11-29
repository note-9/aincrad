#ifndef SHADER_H
#define SHADER_H

#include <iostream>
#include <glad/glad.h>
#include <stdlib.h>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
struct Shader {
    unsigned int id;
};

// Load shader from raw C strings
Shader shader_create(const char* vertexSrc, const char* fragmentSrc);

// Load shader from two text files
Shader shader_from_files(const char* vertexPath, const char* fragmentPath);

void shader_use(Shader* shader);
void shader_destroy(Shader* shader);

void shader_set_bool(Shader* shader, const char* name, bool value);
void shader_set_int(Shader* shader, const char* name, int value);
void shader_set_float(Shader* shader, const char* name, float value);
void shader_set_vec2(Shader* shader, const std::string &name, const glm::vec2 &value);
void shader_set_vec2(Shader* shader, const std::string &name, float x, float y);
void shader_set_vec3(Shader* shader, const std::string &name, const glm::vec3 &value);
void shader_set_vec3(Shader* shader, const std::string &name, float x, float y, float z);
void shader_set_vec4(Shader* shader, const std::string &name, const glm::vec4 &value);
void shader_set_vec4(Shader* shader, const std::string &name, float x, float y, float z, float w);
void shader_set_mat2(Shader* shader, const std::string &name, const glm::mat2 &mat);
void shader_set_mat3(Shader* shader, const std::string &name, const glm::mat3 &mat);

void shader_set_mat4(Shader* shader, const std::string &name, const glm::mat4 &mat);

#endif