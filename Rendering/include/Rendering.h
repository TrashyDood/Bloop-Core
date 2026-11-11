#pragma once

#include "Shared.h"

#include <cstdio>
#include <cstddef>
#include <string>

#define DEFAULT_ASPECT_RATIO Rendering::DefaultAspectRatio
#define ASPECT_RATIO(window) Shared::GetWindowAspectRatio(window)

namespace Rendering
{
    using ResourceHandle = GLuint;

    inline constexpr float DefaultAspectRatio = 16.0f / 9.0f;
    static constexpr std::size_t MaxMeshBufferSize = static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());

    enum ShaderType : GLenum
    {
        VertexShader = GL_VERTEX_SHADER,
        FragmentShader = GL_FRAGMENT_SHADER,
        GeometryShader = GL_GEOMETRY_SHADER
    };

    struct ShaderSource
    {
        const std::string source;
        const GLenum type;
        const bool isFile = false;
    };

    struct Vertex
    {
        Vector3f position{};
        Vector3f normal{};
        Vector2f textureCoordinates{};
        Color4f color = Colors::White;

        Vertex() = default;
        Vertex(Vector3f position, Vector3f normal = {}, Vector2f textureCoordinates = {}, Color4f color = Colors::White)
            : position(position), normal(normal), textureCoordinates(textureCoordinates), color(color) {}
    };

    using MeshVertices = std::vector<Vertex>;
    using MeshIndices = std::vector<std::uint32_t>;

    bool LoadShaderSource(const char *filename, std::string &outSource);
    bool CompileShader(GLenum type, const char *source, ResourceHandle &outShader);
    bool LinkShaders(ResourceHandle &outShaderProgram, const ResourceHandle *shaders, std::size_t shadersCount);
    bool CreateShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources, std::size_t sourcesCount);

    bool CreateMeshBuffers(ResourceHandle &vertexArray, ResourceHandle &vertexBuffer,
        ResourceHandle &indexBuffer, const MeshVertices &vertices, const MeshIndices &indices = {});
}
