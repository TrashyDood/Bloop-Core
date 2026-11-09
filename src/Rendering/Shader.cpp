#include "Rendering/Shader.h"

#include <utility>

bool Rendering::Shader::GetUniformLocation(const std::string &name, GLint &location) const
{
    location = glGetUniformLocation(shaderProgram, name.c_str());
    return location != -1;
}

bool Rendering::Shader::TryGetAttributeLocation(const std::string &name, GLint &location) const
{
    location = glGetAttribLocation(shaderProgram, name.c_str());
    return location != -1;
}

Rendering::Shader &Rendering::Shader::operator=(Shader &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        sources = std::move(other.sources);
        shaderProgram = std::exchange(other.shaderProgram, 0);
    }

    return *this;
}

void Rendering::Shader::ReleaseResources() noexcept
{
    if (shaderProgram != 0)
        glDeleteProgram(shaderProgram);
    shaderProgram = 0;
}

bool Rendering::Shader::InitializeResources()
{
    if (!IResourceOwner::InitializeResources())
        return false;

    if (IsInitialized())
    {
        LOG_ERROR("Cannot initialize shader: program handle is already set.\n");
        return false;
    }

    if (sources.empty())
    {
        LOG_ERROR("Cannot initialize shader: no shader sources provided.\n");
        return false;
    }

    return CreateShaderProgram(shaderProgram, sources.data(), sources.size());
}