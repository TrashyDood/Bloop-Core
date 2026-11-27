#include "Shader.h"

#include "LogHandler.h"

#include <glm/gtc/type_ptr.hpp>
#include <utility>

Graphics::Shader::Shader(std::vector<ShaderSource> sources)
    : sources(std::move(sources)) { }

Graphics::Shader::Shader(Shader &&other) noexcept
    : ResourceOwner(std::move(other)), shaderProgram(std::exchange(other.shaderProgram, 0)),
      sources(std::move(other.sources)),
      memberLocations(std::move(other.memberLocations)) { }

Graphics::Shader &Graphics::Shader::operator=(Shader &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        ResourceOwner::operator=(std::move(other));
        sources = std::move(other.sources);
        shaderProgram = std::exchange(other.shaderProgram, 0);
        memberLocations = std::move(other.memberLocations);
    }

    return *this;
}

bool Graphics::Shader::GetUniformHandle(const std::string &name, ShaderMemberHandle &outHandle)
{
    auto it = memberLocations.find(name);
    if (it != memberLocations.end())
    {
        outHandle = it->second;
        return true;
    }

    outHandle = glGetUniformLocation(shaderProgram, name.c_str());
    if (outHandle < 0) return false;

    memberLocations.emplace(name, outHandle);
    return true;
}

bool Graphics::Shader::GetAttributeHandle(const std::string &name, ShaderMemberHandle &outHandle) const
{
    outHandle = glGetAttribLocation(shaderProgram, name.c_str());
    return outHandle != -1;
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