#pragma once

#include <glad/gl.h>

#include <cstdio>
#include <cstddef>
#include <string>
#include <type_traits>

#define WRITE_ERROR(msg, ...) std::fprintf(stderr, msg, ##__VA_ARGS__)
#define SHADER_SOURCES_TEMPLATE template <typename... Sources, typename = std::enable_if_t<(std::is_same_v<std::decay_t<Sources>, ShaderUtils::ShaderSource> && ...)>>

namespace ShaderUtils
{
    enum ShaderType : GLenum
    {
        Vertex = GL_VERTEX_SHADER,
        Fragment = GL_FRAGMENT_SHADER,
        Geometry = GL_GEOMETRY_SHADER
    };

    enum ShaderLoadReturnCode : int
    {
        Success = 0,
        FileNotFound = 1 << 0,
        ReadError = 1 << 1,
        CompileError = 1 << 2,
        LinkError = 1 << 3,
        InvalidArgument = 1 << 4
    };

    constexpr ShaderLoadReturnCode operator|(ShaderLoadReturnCode lhs, ShaderLoadReturnCode rhs)
    {
        return static_cast<ShaderLoadReturnCode>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    constexpr ShaderLoadReturnCode &operator|=(ShaderLoadReturnCode &lhs, ShaderLoadReturnCode rhs)
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

    ShaderLoadReturnCode TryLoadShaderSource(const char *filename, std::string &outSource);
    ShaderLoadReturnCode TryCompileShader(GLenum type, const char *source, GLuint &outShader);
    ShaderLoadReturnCode TryCreateShaderProgram(GLuint &outShaderProgram, const ShaderSource *sources, std::size_t sourcesCount);

    SHADER_SOURCES_TEMPLATE
    ShaderLoadReturnCode TryCreateShaderProgram(GLuint &outShaderProgram, const Sources &...sources)
    {
        static_assert(sizeof...(sources) > 0, "At least one shader source is required.");

        const ShaderSource shaderSources[] = {{sources.source, sources.type, sources.isFile}...};
        return TryCreateShaderProgram(outShaderProgram, shaderSources, sizeof...(sources));
    }
}
