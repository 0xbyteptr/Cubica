#include "texture.h"
#include <glad/glad.h>

TextureAtlas::~TextureAtlas() {
    if (id) {
        glDeleteTextures(1, &id);
        id = 0;
    }
}
