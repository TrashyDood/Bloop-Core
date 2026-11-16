#pragma once

#include "Shared.h"

#include <cstdio>
#include <cstddef>
#include <string>

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
    using ResourceHandle = GLuint;

    inline constexpr float DefaultAspectRatio = 16.0f / 9.0f;
    inline constexpr std::size_t MaxMeshBufferSize =
        static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());

    struct Vertex
    {
        Vector3f position{};
        Vector3f normal{};
        Vector2f UV{};
        Color4f color = Colors::White;

        Vertex() = default;
        Vertex(Vector3f position, Vector3f normal = {}, Vector2f textureCoordinates = {}, Color4f color = Colors::White)
            : position(position), normal(normal), UV(textureCoordinates), color(color) {}
    };

    enum ShaderType : GLenum
    {
        Unknown = 0,
        VertexShader = GL_VERTEX_SHADER,
        FragmentShader = GL_FRAGMENT_SHADER,
        GeometryShader = GL_GEOMETRY_SHADER
    };

    struct ShaderSource
    {
        const std::string source;
        const GLenum type;
    };

    using MeshVertices = std::vector<Vertex>;
    using MeshIndices = std::vector<UInt32>;

    ShaderType GetShaderFileType(const std::filesystem::path &file);
    std::string GetShaderGroupName(const std::filesystem::path &file);

    void CheckGLErrors();

    bool CompileShader(const GLenum type, const char *source, ResourceHandle &outShader);
    bool LinkShaders(ResourceHandle &outShaderProgram, const ResourceHandle *shaders, const std::size_t shadersCount);
    bool CreateShaderProgram(const ShaderSource *sources, const std::size_t sourcesCount, ResourceHandle &outShaderProgram);

    bool CreateMeshBuffers(ResourceHandle &vertexArray, ResourceHandle &vertexBuffer,
        ResourceHandle &indexBuffer, const MeshVertices &vertices, const MeshIndices &indices = {});
}