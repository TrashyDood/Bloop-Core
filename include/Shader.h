#pragma once

#include "ShaderUtils.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

class Shader
{
    GLuint shaderProgramHandle = 0;

public:
    Shader() = default;
    ~Shader();
    explicit Shader(GLuint shaderProgramHandle) noexcept
        : shaderProgramHandle(shaderProgramHandle) {}

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    Shader(Shader &&other) noexcept;
    Shader &operator=(Shader &&other) noexcept;
    Shader &operator=(GLuint shaderProgramHandle) noexcept;

    bool IsValid() const noexcept { return shaderProgramHandle != 0; }
    GLuint GetShaderHandle() const noexcept { return shaderProgramHandle; }
    void Use() const;

    SHADER_SOURCES_TEMPLATE
    static ShaderUtils::ShaderLoadReturnCode TryCreateNewShader(Shader &outShader, const Sources &...sources)
    {
        GLuint shaderProgram = 0;
        const ShaderUtils::ShaderLoadReturnCode error = ShaderUtils::TryCreateShaderProgram(shaderProgram, sources...);
        if (error != ShaderUtils::Success)
            return error;

        outShader = Shader(shaderProgram);
        return ShaderUtils::Success;
    }
};