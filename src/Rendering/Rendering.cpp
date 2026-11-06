#include "Rendering/Rendering.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

bool Rendering::LoadShaderSource(const char *filename, std::string &outSource)
{
    if (filename == nullptr)
    {
        LOG_ERROR("Cannot load shader source: filename is null.\n");
        return false;
    }

    const std::string sourcePath = std::string(ENGINE_SHADER_DIR) + "/" + filename;
    std::ifstream file(sourcePath);
    if (!file)
    {
        LOG_ERROR("Failed to open shader file: %s\n", sourcePath.c_str());
        return false;
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    if (file.bad())
    {
        LOG_ERROR("Failed to read shader file: %s\n", sourcePath.c_str());
        return false;
    }

    outSource = contents.str();
    return true;
}

bool Rendering::CompileShader(GLenum type, const char *source, ResourceHandle &outShader)
{
    outShader = 0;
    if (source == nullptr)
    {
        LOG_ERROR("Cannot compile shader: source is null.\n");
        return false;
    }

    outShader = glCreateShader(type);
    if (outShader == 0)
    {
        LOG_ERROR("Failed to create shader.\n");
        return false;
    }

    glShaderSource(outShader, 1, &source, nullptr);
    glCompileShader(outShader);

    GLint success = GL_FALSE;
    glGetShaderiv(outShader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        char log[512]{};
        glGetShaderInfoLog(outShader, sizeof(log), nullptr, log);
        LOG_ERROR("Shader compilation failed: %s\n", log);
        glDeleteShader(outShader);
        outShader = 0;
        return false;
    }

    return true;
}

bool Rendering::LinkShaders(ResourceHandle &shaderProgram, const ResourceHandle *shaders, std::size_t shadersCount)
{
    if (shaders == nullptr || shadersCount == 0)
    {
        LOG_ERROR("Cannot link shader program: no shader handles provided.\n");
        return false;
    }

    for (std::size_t i = 0; i < shadersCount; ++i)
        if (shaders[i] == 0)
        {
            LOG_ERROR("Cannot link shader program: shader handle at index %zu is invalid.\n", i);
            return false;
        }

    if (shaderProgram == 0) shaderProgram = glCreateProgram();
    if (shaderProgram == 0)
    {
        LOG_ERROR("Failed to create shader program.\n");
        return false;
    }

    for (std::size_t i = 0; i < shadersCount; ++i)
        glAttachShader(shaderProgram, shaders[i]);

    glLinkProgram(shaderProgram);

    GLint success = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    for (std::size_t i = 0; i < shadersCount; ++i)
        glDetachShader(shaderProgram, shaders[i]);

    if (success == GL_FALSE)
    {
        char log[512]{};
        glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
        LOG_ERROR("Shader link failed: %s\n", log);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return false;
    }

    return true;
}

bool Rendering::CreateShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources, std::size_t sourcesCount)
{
    outShaderProgram = 0;
    if (sources == nullptr || sourcesCount == 0)
    {
        LOG_ERROR("Cannot create shader program: no shader sources provided.\n");
        return false;
    }

    std::vector<ResourceHandle> shaders(sourcesCount, 0);
    std::vector<std::string> loadedSources(sourcesCount);

    for (std::size_t i = 0; i < sourcesCount; ++i)
    {
        const std::string *source = &sources[i].source;
        if (sources[i].isFile)
        {
            if (!LoadShaderSource(sources[i].source.c_str(), loadedSources[i]))
            {
                for (const ResourceHandle shader : shaders)
                    if (shader != 0)
                        glDeleteShader(shader);
                return false;
            }
            source = &loadedSources[i];
        }

        if (!CompileShader(sources[i].type, source->c_str(), shaders[i]))
        {
            for (const ResourceHandle shader : shaders)
            {
                if (shader != 0)
                    glDeleteShader(shader);
            }
            return false;
        }
    }

    const bool linkSucceeded = LinkShaders(outShaderProgram, shaders.data(), shaders.size());

    for (const ResourceHandle shader : shaders)
        glDeleteShader(shader);

    return linkSucceeded;
}
