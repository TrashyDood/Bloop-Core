#include "Shader.h"

Shader::~Shader()
{
    if (IsValid())
        glDeleteProgram(shaderProgramHandle);
}

Shader::Shader(Shader &&other) noexcept
    : shaderProgramHandle(other.shaderProgramHandle)
{
    other.shaderProgramHandle = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept
{
    if (this != &other)
    {
        if (IsValid())
            glDeleteProgram(shaderProgramHandle);

        shaderProgramHandle = other.shaderProgramHandle;
        other.shaderProgramHandle = 0;
    }

    return *this;
}

Shader &Shader::operator=(GLuint shaderProgramHandle) noexcept
{
    if (IsValid())
        glDeleteProgram(shaderProgramHandle);

    this->shaderProgramHandle = shaderProgramHandle;
    return *this;
}

void Shader::Use() const
{
    glUseProgram(shaderProgramHandle);
}