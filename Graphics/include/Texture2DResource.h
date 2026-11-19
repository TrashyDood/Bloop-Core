#pragma once

#include "Graphics.h"
#include "ResourceOwner.h"

#include <vector>

#define TEXTURE_MAX_WIDTH 8192
#define TEXTURE_MAX_HEIGHT 8192

namespace Graphics
{
    class Texture2DResource : public ResourceOwner
    {
        ResourceHandle texture = 0;
        Int32 width,
            height;
        std::vector<UInt8> pixels;

    public:
        [[nodiscard]] ResourceHandle GetTextureHandle() const { return texture; }
        [[nodiscard]] bool IsInitialized() const override;

        Texture2DResource() = default;
        Texture2DResource(Int32 width, Int32 height, std::vector<UInt8> rgbaPixels);

        Texture2DResource(Texture2DResource &&other) noexcept;
        Texture2DResource &operator=(Texture2DResource &&other) noexcept;

        ~Texture2DResource() override;

        Texture2DResource(const Texture2DResource &) = delete;
        Texture2DResource &operator=(const Texture2DResource &) = delete;

        bool InitializeResources() override;
        void Bind(GLenum textureUnit = GL_TEXTURE0) const;

    private:
        void ReleaseResources() noexcept override;
    };
}
