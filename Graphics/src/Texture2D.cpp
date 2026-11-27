#include "Texture2D.h"

#include "LogHandler.h"

#include <utility>

Graphics::Texture2D::Texture2D(
    Int32 width, Int32 height, std::vector<UInt8> rgbaPixels)
    : width(width), height(height), pixels(std::move(rgbaPixels)) { }

Graphics::Texture2D::Texture2D(Texture2D &&other) noexcept
    : ResourceOwner(std::move(other)), texture(std::exchange(other.texture, 0)), width(std::exchange(other.width, 0)),
      height(std::exchange(other.height, 0)), pixels(std::move(other.pixels)) { }

Graphics::Texture2D &Graphics::Texture2D::operator=(Texture2D &&other) noexcept
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

void Graphics::Texture2D::ReleaseResources() noexcept
{
    if (texture != 0)
        glDeleteTextures(1, &texture);
    texture = 0;
}

bool Graphics::Texture2D::InitializeResources()
{
    LOG_SCOPE;

    if (!ResourceOwner::InitializeResources())
        return false;

    if (!Graphics::CreateGLTexture2D(texture,
        { pixels, static_cast<UInt32>(width), static_cast<UInt32>(height) }))
        return false;

    return true;
}

void Graphics::Texture2D::Bind(const GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
}
