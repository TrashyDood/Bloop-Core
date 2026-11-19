#include "Texture2DResource.h"

#include "ResourceUtils.h"
#include "LogHandler.h"

#include <utility>

Graphics::Texture2DResource::Texture2DResource(
    Int32 width, Int32 height, std::vector<UInt8> rgbaPixels)
    : width(width), height(height), pixels(std::move(rgbaPixels)) { }

Graphics::Texture2DResource::Texture2DResource(Texture2DResource &&other) noexcept
    : ResourceOwner(std::move(other)), texture(std::exchange(other.texture, 0)), width(std::exchange(other.width, 0)),
      height(std::exchange(other.height, 0)), pixels(std::move(other.pixels)) { }

Graphics::Texture2DResource &Graphics::Texture2DResource::operator=(Texture2DResource &&other) noexcept
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

Graphics::Texture2DResource::~Texture2DResource()
{
    ReleaseResources();
}

bool Graphics::Texture2DResource::IsInitialized() const
{
    return texture != 0;
}

void Graphics::Texture2DResource::ReleaseResources() noexcept
{
    if (texture != 0)
        glDeleteTextures(1, &texture);
    texture = 0;
}

bool Graphics::Texture2DResource::InitializeResources()
{
    LOG_SCOPE;

    if (!ResourceOwner::InitializeResources())
        return false;

    if (!Graphics::CreateGLTexture2D(texture,
        { pixels, static_cast<UInt32>(width), static_cast<UInt32>(height) }))
        return false;

    return true;
}

void Graphics::Texture2DResource::Bind(const GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
}
