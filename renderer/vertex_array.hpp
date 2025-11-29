#pragma once
#include <glad/glad.h>

struct VertexArray {
    GLuint id;

    void init();
    void bind();
    void unbind();
    void destroy();
    void enableAttrib(unsigned int index, int size, GLenum type, bool normalized, int stride, const void* pointer);
};
