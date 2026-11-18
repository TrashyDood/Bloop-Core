#include "Graphics.h"
#include "LogHandler.h"

Graphics::ShaderType Graphics::GetShaderFileType(const std::filesystem::path &file)
{
    const std::string stage = file.stem().extension().string();
    const ShaderType stageType = ShaderTypeFromString(stage);
    if (stageType != ShaderType::Unknown)
        return stageType;

    const std::string name = file.stem().string();
    return ShaderTypeFromString(name);
}

std::string Graphics::GetShaderGroupName(const std::filesystem::path &file)
{
    const std::filesystem::path stem = file.stem();
    return stem.extension().empty() ? "default" : stem.stem().string();
}

void Graphics::CheckGLErrors()
{
    LOG_SCOPE;

    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        LOG_ERROR("OpenGL error: 0x%X\n", error);
}