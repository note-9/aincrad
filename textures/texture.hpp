#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <stb_image.hpp>

struct Texture
{
    unsigned int id;
    int width;
    int height;
    int channels;
};

Texture texture_load(const char* path, GLuint texture_unit);
void texture_bind(const Texture* tex, GLuint texture_unit);
void texture_destroy(Texture* tex);

#endif
