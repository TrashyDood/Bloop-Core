#include "ShaderUtils.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

ShaderUtils::ShaderLoadReturnCode ShaderUtils::TryLoadShaderSource(const char *filename, std::string &outSource)
{
    if (filename == nullptr)
        return InvalidArgument;

    const std::string sourcePath = std::string(ENGINE_SHADER_DIR) + "/" + filename;
    std::ifstream file(sourcePath);
    if (!file)
    {
        std::fprintf(stderr, "Failed to open shader file: %s\n", sourcePath.c_str());
        return FileNotFound;
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    if (file.bad())
    {
        std::fprintf(stderr, "Failed to read shader file: %s\n", sourcePath.c_str());
        return ReadError;
    }

    outSource = contents.str();
    return Success;
}

ShaderUtils::ShaderLoadReturnCode ShaderUtils::TryCompileShader(GLenum type, const char *source, GLuint &outShader)
{
    outShader = 0;
    if (source == nullptr)
        return InvalidArgument;

    outShader = glCreateShader(type);
    if (outShader == 0)
        return CompileError;

    glShaderSource(outShader, 1, &source, nullptr);
    glCompileShader(outShader);

    GLint success = GL_FALSE;
    glGetShaderiv(outShader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        char log[512]{};
        glGetShaderInfoLog(outShader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader compilation failed: %s\n", log);
        glDeleteShader(outShader);
        outShader = 0;
        return CompileError;
    }

    return Success;
}

ShaderUtils::ShaderLoadReturnCode ShaderUtils::TryCreateShaderProgram(
    GLuint &outShaderProgram,
    const ShaderSource *sources,
    std::size_t sourcesCount)
{
    outShaderProgram = 0;
    if (sources == nullptr || sourcesCount == 0)
        return InvalidArgument;

    std::vector<GLuint> shaders(sourcesCount, 0);
    std::vector<std::string> loadedSources(sourcesCount);
    ShaderLoadReturnCode error = Success;

    for (std::size_t i = 0; i < sourcesCount; ++i)
    {
        const std::string *source = &sources[i].source;
        if (sources[i].isFile)
        {
            const ShaderLoadReturnCode loadError = TryLoadShaderSource(sources[i].source.c_str(), loadedSources[i]);
            if (loadError != Success)
            {
                error |= loadError;
                continue;
            }
            source = &loadedSources[i];
        }

        error |= TryCompileShader(sources[i].type, source->c_str(), shaders[i]);
    }

    if (error != Success)
    {
        for (const GLuint shader : shaders)
            if (shader != 0)
                glDeleteShader(shader);
        return error;
    }

    outShaderProgram = glCreateProgram();
    if (outShaderProgram == 0)
    {
        for (const GLuint shader : shaders)
            glDeleteShader(shader);
        return LinkError;
    }

    for (const GLuint shader : shaders)
        glAttachShader(outShaderProgram, shader);

    glLinkProgram(outShaderProgram);

    GLint success = GL_FALSE;
    glGetProgramiv(outShaderProgram, GL_LINK_STATUS, &success);

    for (const GLuint shader : shaders)
    {
        glDetachShader(outShaderProgram, shader);
        glDeleteShader(shader);
    }

    if (success == GL_FALSE)
    {
        char log[512]{};
        glGetProgramInfoLog(outShaderProgram, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader link failed: %s\n", log);
        glDeleteProgram(outShaderProgram);
        outShaderProgram = 0;
        return LinkError;
    }

    return Success;
}
