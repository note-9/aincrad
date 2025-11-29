#include "shader.hpp"
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

static char* read_file(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Failed to open shader file: %s\n", path);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    
    char* buffer = (char*)malloc(size + 1);
    buffer[size] = '\0';
    
    fread(buffer, 1, size, file);
    fclose(file);
    
    return buffer;
}

static unsigned int compile_shader(unsigned int type, const char* src)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        printf("Shader compile error: %s\n", log);
    }
    
    return shader;
}

struct Shader shader_create(const char* vertexSrc, const char* fragmentSrc)
{
    unsigned int vert = compile_shader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int frag = compile_shader(GL_FRAGMENT_SHADER, fragmentSrc);
    
    struct Shader shader;
    shader.id = glCreateProgram();
    
    glAttachShader(shader.id, vert);
    glAttachShader(shader.id, frag);
    glLinkProgram(shader.id);
    
    glDeleteShader(vert);
    glDeleteShader(frag);
    
    return shader;
}

struct Shader shader_from_files(const char* vertexPath, const char* fragmentPath)
{
    char* vertSrc = read_file(vertexPath);
    char* fragSrc = read_file(fragmentPath);
    
    struct Shader shader = shader_create(vertSrc, fragSrc);
    
    free(vertSrc);
    free(fragSrc);
    
    return shader;
}

void shader_use(struct Shader* shader)
{
    glUseProgram(shader->id);
}

void shader_destroy(struct Shader* shader)
{
    glDeleteProgram(shader->id);
}

void shader_set_bool(Shader* shader, const char* name, bool value)
{
    glUniform1i(glGetUniformLocation(shader->id, name), (int)value);
}

void shader_set_int(Shader* shader, const char* name, int value)
{
    glUniform1i(glGetUniformLocation(shader->id, name), value);
}

void shader_set_float(Shader* shader, const char* name, float value)
{
    glUniform1f(glGetUniformLocation(shader->id, name), value);
}
void shader_set_vec2(Shader* shader, const std::string &name, const glm::vec2 &value)
{ 
    glUniform2fv(glGetUniformLocation(shader->id, name.c_str()), 1, &value[0]); 
}
void shader_set_vec2(Shader* shader, const std::string &name, float x, float y)
{ 
    glUniform2f(glGetUniformLocation(shader->id, name.c_str()), x, y); 
}
// ------------------------------------------------------------------------
void shader_set_vec3(Shader* shader, const std::string &name, const glm::vec3 &value)
{ 
    glUniform3fv(glGetUniformLocation(shader->id, name.c_str()), 1, &value[0]); 
}
void shader_set_vec3(Shader* shader, const std::string &name, float x, float y, float z)
{ 
    glUniform3f(glGetUniformLocation(shader->id, name.c_str()), x, y, z); 
}
// ------------------------------------------------------------------------
void shader_set_vec4(Shader* shader, const std::string &name, const glm::vec4 &value)
{ 
    glUniform4fv(glGetUniformLocation(shader->id, name.c_str()), 1, &value[0]); 
}
void shader_set_vec4(Shader* shader, const std::string &name, float x, float y, float z, float w)
{ 
    glUniform4f(glGetUniformLocation(shader->id, name.c_str()), x, y, z, w); 
}
// ------------------------------------------------------------------------
void shader_set_mat2(Shader* shader, const std::string &name, const glm::mat2 &mat)
{
    glUniformMatrix2fv(glGetUniformLocation(shader->id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void shader_set_mat3(Shader* shader, const std::string &name, const glm::mat3 &mat)
{
    glUniformMatrix3fv(glGetUniformLocation(shader->id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void shader_set_mat4(Shader* shader, const std::string &name, const glm::mat4 &mat)
{
    glUniformMatrix4fv(glGetUniformLocation(shader->id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
