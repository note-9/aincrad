#pragma once
#include <glad/glad.h>

struct ElementBuffer {
    GLuint id;

    void init(const void* data, unsigned int size);
    void bind();
    void unbind();
    void destroy();
};
