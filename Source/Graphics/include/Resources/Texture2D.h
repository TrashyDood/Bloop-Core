#pragma once

#include "Graphics.h"
#include "ResourceOwner.h"

#include <vector>

#define TEXTURE_MAX_WIDTH 8192
#define TEXTURE_MAX_HEIGHT 8192

namespace Graphics
{
    enum class TextureFormat : GLenum
    {
        RGBA8       = GL_RGBA8,
        RGB8        = GL_RGB8,
        RGBA16F     = GL_RGBA16F,
        RGB16F      = GL_RGB16F,
        RGBA32F     = GL_RGBA32F,
        RGB32F      = GL_RGB32F
    };

    enum class TextureFilterMode : GLenum
    {
        Nearest                 = GL_NEAREST,
        Linear                  = GL_LINEAR,
        NearestMipmapNearest    = GL_NEAREST_MIPMAP_NEAREST,
        LinearMipmapNearest     = GL_LINEAR_MIPMAP_NEAREST,
        NearestMipmapLinear     = GL_NEAREST_MIPMAP_LINEAR,
        LinearMipmapLinear      = GL_LINEAR_MIPMAP_LINEAR
    };

    enum class TextureWrap : GLenum
    {
        Repeat              = GL_REPEAT,
        MirroredRepeat      = GL_MIRRORED_REPEAT,
        ClampToEdge         = GL_CLAMP_TO_EDGE,
        ClampToBorder       = GL_CLAMP_TO_BORDER
    };
    
    struct TextureData
    {
        std::vector<UInt8> pixels;
        UInt32 width = 0;
        UInt32 height = 0;
        TextureWrap wrapMode                = TextureWrap::Repeat;
        TextureFilterMode filterMode        = TextureFilterMode::Linear;
        TextureFormat format                = TextureFormat::RGBA8;
    };

    bool CreateGLTexture2D(ResourceHandle &outTexture, const TextureData &textureData);

    class Texture2D : public ResourceOwner
    {
        ResourceHandle texture = 0;
        TextureData data;

    public:
        [[nodiscard]] ResourceHandle GetTextureHandle() const noexcept  { return texture; }
        [[nodiscard]] bool IsInitialized() const noexcept override      { return texture != 0; }

        Texture2D() = default;
        explicit Texture2D(TextureData textureData);
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
