#pragma once

#include "Shared.h"

#include <cstdio>
#include <cstddef>
#include <string>
#include <string_view>

#include <glad/gl.h>

#define DEFAULT_ASPECT_RATIO Graphics::DefaultAspectRatio
#define MAX_MESH_BUFFER_SIZE Graphics::MaxMeshBufferSize

#define SHADER_EXTENSION ".glsl"
#define PNG_EXTENSION ".png"
#define OBJ_EXTENSION ".obj"

#define ASSETS_DIR WORKING_DIR / "assets"
#define MODELS_DIR ASSETS_DIR / "models"
#define SHADERS_DIR ASSETS_DIR / "shaders"
#define TEXTURES_DIR ASSETS_DIR / "textures"

namespace Graphics
{
    enum class TextureFormat : GLenum
    {
        RGBA8 =     GL_RGBA8,
        RGB8 =      GL_RGB8,
        RGBA16F =   GL_RGBA16F,
        RGB16F =    GL_RGB16F,
        RGBA32F =   GL_RGBA32F,
        RGB32F =    GL_RGB32F
    };

    enum class TextureFilter : GLenum
    {
        Nearest =               GL_NEAREST,
        Linear =                GL_LINEAR,
        NearestMipmapNearest =  GL_NEAREST_MIPMAP_NEAREST,
        LinearMipmapNearest =   GL_LINEAR_MIPMAP_NEAREST,
        NearestMipmapLinear =   GL_NEAREST_MIPMAP_LINEAR,
        LinearMipmapLinear =    GL_LINEAR_MIPMAP_LINEAR
    };

    enum class TextureWrap : GLenum
    {
        Repeat =            GL_REPEAT,
        MirroredRepeat =    GL_MIRRORED_REPEAT,
        ClampToEdge =       GL_CLAMP_TO_EDGE,
        ClampToBorder =     GL_CLAMP_TO_BORDER
    };

    enum class ShaderType : GLenum
    {
        Unknown = 0,
        VertexShader = GL_VERTEX_SHADER,
        FragmentShader = GL_FRAGMENT_SHADER,
        GeometryShader = GL_GEOMETRY_SHADER
    };

    struct Vertex
    {
        Vector3f position{};
        Vector3f normal{};
        Vector2f UV{};
        Color4f color = Colors::White;
    };

    struct ShaderSource
    {
        std::string source;
        ShaderType type;
    };
    
    struct TextureData
    {
        std::vector<UInt8>pixels;
        UInt32 width = 0;
        UInt32 height = 0;
        TextureWrap wrapMode =        TextureWrap::Repeat;
        TextureFilter filterMode =    TextureFilter::Linear;
        TextureFormat format =        TextureFormat::RGBA8;
    };

    using MeshVertices =    std::vector<Vertex>;
    using MeshIndices =     std::vector<UInt32>;
    using ResourceHandle =  GLuint;

    inline constexpr float DefaultAspectRatio =         16.0f / 9.0f;
    inline constexpr std::size_t MaxMeshBufferSize =    static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());

    inline constexpr const char *ShaderTypeToString(ShaderType type)
    {
        switch (type)
        {
            case ShaderType::VertexShader: return "vertex";
            case ShaderType::FragmentShader: return "fragment";
            case ShaderType::GeometryShader: return "geometry";
            default: return "unknown";
        }
    }

    inline constexpr ShaderType ShaderTypeFromString(std::string_view stage)
    {
        if (stage == ".vert" || stage == ".vs" || stage == ".vertex" ||
            stage == "vertex" || stage == "vert" || stage == "vs")
            return ShaderType::VertexShader;
        if (stage == ".frag" || stage == ".fs" || stage == ".fragment" ||
            stage == "fragment" || stage == "frag" || stage == "fs")
            return ShaderType::FragmentShader;
        if (stage == ".geom" || stage == ".gs" || stage == ".geometry" ||
            stage == "geometry" || stage == "geom" || stage == "gs")
            return ShaderType::GeometryShader;
        return ShaderType::Unknown;
    }

    ShaderType GetShaderFileType(const std::filesystem::path &file);
    std::string GetShaderGroupName(const std::filesystem::path &file);

    void CheckGLErrors();
}