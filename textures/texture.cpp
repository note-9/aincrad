#include "texture.hpp"
#include <iostream>

Texture texture_load(const char* path, GLuint texture_unit)
{
    Texture tex = {};
    glGenTextures(1, &tex.id);

    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load image
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &tex.width, &tex.height, &tex.channels, 0);

    if (!data)
    {
        std::cout << "Failed to load texture: " << path << "\n";
        return tex;
    }

    GLenum format = (tex.channels == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format,
                 tex.width, tex.height, 0,
                 format, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    return tex;
}

void texture_bind(const Texture* tex, GLuint texture_unit)
{
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, tex->id);
}

void texture_destroy(Texture* tex)
{
    glDeleteTextures(1, &tex->id);
    tex->id = 0;
}
