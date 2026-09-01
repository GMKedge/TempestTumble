#include "Texture.hpp"
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

namespace tempest {

bool Texture::loadFromFile(const std::string& path) {
    stbi_set_flip_vertically_on_load(0); // PNG row 0 stays GPU row 0; dest top uses atlas v0
    int n = 0;
    unsigned char* data = stbi_load(path.c_str(), &w_, &h_, &n, 4);
    if (!data) {
        std::cerr << "stb_image failed: " << path << "\n";
        return false;
    }
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_, h_, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return true;
}

void Texture::destroy() {
    if (id_) { glDeleteTextures(1, &id_); id_ = 0; }
}

void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

} // namespace tempest
