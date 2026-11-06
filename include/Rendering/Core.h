#pragma once

#include "Shared/Core.h"

#include <cstdio>
#include <cstddef>
#include <string>
#include <type_traits>

namespace Rendering
{
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

    bool LoadShaderSource(const char *filename, std::string &outSource);
    bool CompileShader(GLenum type, const char *source, ResourceHandle &outShader);
    bool LinkShaders(ResourceHandle &outShaderProgram, const ResourceHandle *shaders, std::size_t shadersCount);
    bool CreateShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources, std::size_t sourcesCount);
}
