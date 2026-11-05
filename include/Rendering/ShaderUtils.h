#pragma once

#include "Shared/Core.h"

#include <cstdio>
#include <cstddef>
#include <string>
#include <type_traits>

#define SHADER_SOURCES_TEMPLATE template <typename... Sources, typename = std::enable_if_t<(std::is_same_v<std::decay_t<Sources>, ShaderUtils::ShaderSource> && ...)>>

namespace ShaderUtils
{
    enum ShaderType : GLenum
    {
        Vertex = GL_VERTEX_SHADER,
        Fragment = GL_FRAGMENT_SHADER,
        Geometry = GL_GEOMETRY_SHADER
    };

    enum ShaderReturnCode : unsigned int
    {
        None = 0,
        FileNotFound = 1 << 0,
        ReadError = 1 << 1,
        CompileError = 1 << 2,
        LinkError = 1 << 3,
        InvalidArgument = 1 << 4
    };

    constexpr ShaderReturnCode operator|(ShaderReturnCode lhs, ShaderReturnCode rhs)
    {
        return static_cast<ShaderReturnCode>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    constexpr ShaderReturnCode &operator|=(ShaderReturnCode &lhs, ShaderReturnCode rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    struct ShaderSource
    {
        std::string source;
        GLenum type;
        bool isFile = false;
    };

    ShaderReturnCode LoadShaderSource(const char *filename, std::string &outSource);
    ShaderReturnCode CompileShader(GLenum type, const char *source, ResourceHandle &outShader);
    ShaderReturnCode LinkShaders(ResourceHandle &outShaderProgram, const ResourceHandle *shaders, std::size_t shadersCount);
    ShaderReturnCode CreateShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources, std::size_t sourcesCount);

    SHADER_SOURCES_TEMPLATE
    ShaderReturnCode CreateShaderProgram(ResourceHandle &outShaderProgram, const Sources &...sources)
    {
        static_assert(sizeof...(sources) > 0, "At least one shader source is required.");

        const ShaderSource shaderSources[] = {{sources.source, sources.type, sources.isFile}...};
        return CreateShaderProgram(outShaderProgram, shaderSources, sizeof...(sources));
    }
}
