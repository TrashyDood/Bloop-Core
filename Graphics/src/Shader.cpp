#include "Shader.h"

#include "LogHandler.h"

#include <utility>

Graphics::Shader::Shader(std::vector<ShaderSource> sources)
    : sources(std::move(sources)) { }

Graphics::Shader::Shader(Shader &&other) noexcept
    : ResourceOwner(std::move(other)), shaderProgram(std::exchange(other.shaderProgram, 0)),
      sources(std::move(other.sources)) { }

bool Graphics::Shader::GetUniformLocation(const std::string &name, GLint &location) const
{
    location = glGetUniformLocation(shaderProgram, name.c_str());
    return location != -1;
}

bool Graphics::Shader::GetAttributeLocation(const std::string &name, GLint &location) const
{
    location = glGetAttribLocation(shaderProgram, name.c_str());
    return location != -1;
}

Graphics::Shader &Graphics::Shader::operator=(Shader &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        ResourceOwner::operator=(std::move(other));
        sources = std::move(other.sources);
        shaderProgram = std::exchange(other.shaderProgram, 0);
    }

    return *this;
}

Graphics::Shader::~Shader()
{
    ReleaseResources();
}

bool Graphics::Shader::IsInitialized() const
{
    return shaderProgram != 0;
}

void Graphics::Shader::ReleaseResources() noexcept
{
    if (shaderProgram != 0)
        glDeleteProgram(shaderProgram);
    shaderProgram = 0;
}

bool Graphics::Shader::InitializeResources()
{
    LOG_SCOPE;

    if (!ResourceOwner::InitializeResources())
        return false;

    if (sources.empty())
    {
        LOG_ERROR("Cannot initialize shader: no shader sources provided.\n");
        return false;
    }

    if (!CreateGLShaderProgram(shaderProgram, sources.data(), sources.size()))
        return false;

    return true;
}