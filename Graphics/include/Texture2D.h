#pragma once

#include "Graphics.h"
#include "ResourceOwner.h"

#include <vector>

#define TEXTURE_MAX_WIDTH 8192
#define TEXTURE_MAX_HEIGHT 8192

namespace Graphics
{
    class Texture2D : public ResourceOwner
    {
        ResourceHandle texture = 0;
        Int32 width, height;
        std::vector<UInt8> pixels;

    public:
        [[nodiscard]] ResourceHandle GetTextureHandle() const       { return texture; }
        [[nodiscard]] bool IsInitialized() const noexcept override  { return texture != 0; }

        Texture2D() = default;
        Texture2D(Int32 width, Int32 height, std::vector<UInt8> rgbaPixels);
        ~Texture2D() { ReleaseResources(); }

        Texture2D(Texture2D &&other) noexcept;
        Texture2D &operator=(Texture2D &&other) noexcept;
        Texture2D(const Texture2D &) = delete;
        Texture2D &operator=(const Texture2D &) = delete;

        bool InitializeResources() override;
        void Bind(GLenum textureUnit = GL_TEXTURE0) const;

    private:
        void ReleaseResources() noexcept override;
    };
}
