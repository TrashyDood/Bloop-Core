#include "Rendering/Shader.h"

#include <utility>

Rendering::Shader::~Shader()
{
    ReleaseResources();
}

Rendering::Shader::Shader(Shader &&other) noexcept
    : shaderProgram(std::exchange(other.shaderProgram, 0)), sources(std::move(other.sources)) { }

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

bool Rendering::Shader::InitializeResources(bool reinitialize)
{
    if (reinitialize)
        ReleaseResources();

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