#pragma once

#include "Graphics.h"
#include "IResourceOwner.h"

#include <utility>
#include <vector>

#define TEXTURE_MAX_WIDTH 8192
#define TEXTURE_MAX_HEIGHT 8192

namespace Graphics
{
    class TextureResource : public ResourceOwner
    {
        ResourceHandle texture = 0;
        Int32 width;
        Int32 height;
        std::vector<UInt8> pixels;

    public:
        [[nodiscard]] ResourceHandle GetTextureHandle() const { return texture; }
        [[nodiscard]] bool IsInitialized() const override { return texture != 0; }

        TextureResource(Int32 width, Int32 height, std::vector<UInt8> rgbaPixels)
            : width(width), height(height), pixels(std::move(rgbaPixels)) {}

        TextureResource(TextureResource &&other) noexcept;
        TextureResource &operator=(TextureResource &&other) noexcept;

        ~TextureResource() override { ReleaseResources(); }

        TextureResource(const TextureResource &) = delete;
        TextureResource &operator=(const TextureResource &) = delete;

        bool InitializeResources() override;
        void Bind(GLenum textureUnit = GL_TEXTURE0) const;

    private:
        void ReleaseResources() noexcept override;
    };
}
