#include "Texture2D.h"

#include "LogHandler.h"

#include <utility>

Graphics::Texture2D::Texture2D(TextureData textureData)
    : data(std::move(textureData)) { }

Graphics::Texture2D::Texture2D(Texture2D &&other) noexcept
    : ResourceOwner(std::move(other)), texture(std::exchange(other.texture, 0)), data(std::move(other.data)) { }

Graphics::Texture2D &Graphics::Texture2D::operator=(Texture2D &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        ResourceOwner::operator=(std::move(other));
        texture = std::exchange(other.texture, 0);
        data = std::move(other.data);
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

    if (!Graphics::CreateGLTexture2D(texture, data))
        return false;

    return true;
}

void Graphics::Texture2D::Bind(const GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

bool Graphics::CreateGLTexture2D(ResourceHandle &outTexture, const TextureData &textureData)
{
    LOG_SCOPE;

    if (outTexture != 0)
    {
        LOG_ERROR("Cannot create texture: output texture handle is already initialized.\n");
        return false;
    }

    if (textureData.pixels.empty())
    {
        LOG_ERROR("Cannot create texture: pixel data is empty.\n");
        return false;
    }

    // Calculate the pixel width and height based on the aspect ratio and pixel count
    const float aspect = static_cast<float>(textureData.width) / static_cast<float>(textureData.height);
    const UInt32 pixelWidth =
        std::roundf(std::sqrt(static_cast<float>(textureData.pixels.size()) / 4.0f * aspect));
    const UInt32 pixelHeight = std::roundf(static_cast<float>(pixelWidth) / aspect);

    if (aspect <= 0 || pixelWidth < 1 || pixelHeight < 1)
    {
        LOG_ERROR("Cannot create texture: invalid dimensions.\n");
        return false;
    }

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (pixelWidth > maxTextureSize || pixelHeight > maxTextureSize)
    {
        LOG_ERROR("Cannot create texture: dimensions exceed the texture size limit.\n");
        return false;
    }

    glGenTextures(1, &outTexture);
    if (outTexture == 0)
    {
        LOG_ERROR("Cannot create texture: failed to allocate texture.\n");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, outTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(textureData.wrapMode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(textureData.wrapMode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(textureData.filterMode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(textureData.filterMode));
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(textureData.format), textureData.width, textureData.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, textureData.pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}
