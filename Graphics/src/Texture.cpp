#include "Texture.h"
#include "LogHandler.h"

#include <limits>
#include <utility>

Graphics::TextureResource::TextureResource(TextureResource &&other) noexcept
    : ResourceOwner(std::move(other)), texture(std::exchange(other.texture, 0)), width(std::exchange(other.width, 0)),
      height(std::exchange(other.height, 0)), pixels(std::move(other.pixels)) { }

Graphics::TextureResource &Graphics::TextureResource::operator=(TextureResource &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        ResourceOwner::operator=(std::move(other));
        texture = std::exchange(other.texture, 0);
        width = std::exchange(other.width, 0);
        height = std::exchange(other.height, 0);
        pixels = std::move(other.pixels);
    }

    return *this;
}

void Graphics::TextureResource::ReleaseResources() noexcept
{
    if (texture != 0)
        glDeleteTextures(1, &texture);
    texture = 0;
}

bool Graphics::TextureResource::InitializeResources()
{
    if (!ResourceOwner::InitializeResources())
        return false;

    if (width <= 0 || height <= 0 ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / 4 / static_cast<std::size_t>(height) ||
        pixels.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4)
    {
        LOG_ERROR("Cannot initialize texture: dimensions or RGBA pixel data are invalid.\n");
        return false;
    }

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (width > maxTextureSize || height > maxTextureSize)
    {
        LOG_ERROR("Cannot initialize texture: dimensions exceed the OpenGL texture size limit.\n");
        return false;
    }

    glGenTextures(1, &texture);
    if (texture == 0)
    {
        LOG_ERROR("Cannot initialize texture: OpenGL failed to allocate a texture.\n");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    LOG_INFO("Initialized texture: %d x %d pixels.\n", width, height);
    return true;
}

void Graphics::TextureResource::Bind(const GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
}
