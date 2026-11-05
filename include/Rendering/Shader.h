#pragma once

#include "Rendering/ShaderUtils.h"
#include "Shared/Core.h"

class Shader
{
    ResourceHandle shaderProgramHandle = 0;

public:
    Shader() = default;
    ~Shader();
    explicit Shader(ResourceHandle shaderProgramHandle) noexcept
        : shaderProgramHandle(shaderProgramHandle) {}

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    Shader(Shader &&other) noexcept;
    Shader &operator=(Shader &&other) noexcept;
    Shader &operator=(ResourceHandle shaderProgramHandle) noexcept;

    bool IsValid() const noexcept { return shaderProgramHandle != 0; }
    ResourceHandle GetShaderHandle() const noexcept { return shaderProgramHandle; }
    void Use() const;

    SHADER_SOURCES_TEMPLATE
    static ShaderUtils::ShaderReturnCode TryCreateNewShader(Shader &outShader, const Sources &...sources)
    {
        ResourceHandle shaderProgram = 0;
        const auto error = ShaderUtils::CreateShaderProgram(shaderProgram, sources...);
        if (error != ShaderUtils::None)
            return error;

        outShader = Shader(shaderProgram);
        return ShaderUtils::None;
    }
};