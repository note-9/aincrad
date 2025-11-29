#include "vertex_array.hpp"

void VertexArray::init() {
    glGenVertexArrays(1, &id);
}

void VertexArray::bind() {
    glBindVertexArray(id);
}

void VertexArray::unbind() {
    glBindVertexArray(0);
}

void VertexArray::destroy() {
    glDeleteVertexArrays(1, &id);
}

void VertexArray::enableAttrib(unsigned int index, int size, GLenum type, bool normalized, int stride, const void* pointer) {
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}
