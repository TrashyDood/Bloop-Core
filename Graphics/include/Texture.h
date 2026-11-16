#pragma once

#include "Graphics.h"
#include "IResourceOwner.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace Graphics
{
    class TextureResource : public ResourceOwner
    {
        ResourceHandle texture = 0;
        int width;
        int height;
        std::vector<std::uint8_t> pixels;

    public:
        [[nodiscard]] ResourceHandle GetTextureHandle() const { return texture; }
        [[nodiscard]] bool IsInitialized() const override { return texture != 0; }

        TextureResource(int width, int height, std::vector<std::uint8_t> rgbaPixels)
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
