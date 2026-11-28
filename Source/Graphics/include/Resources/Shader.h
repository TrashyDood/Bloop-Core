#pragma once

#include "Graphics.h"
#include "Shared.h"
#include "ResourceOwner.h"

#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <unordered_map>
#include <type_traits>

namespace Graphics
{
    enum class ShaderSourceType : GLenum
    {
        Unknown = 0,
        VertexShader = GL_VERTEX_SHADER,
        FragmentShader = GL_FRAGMENT_SHADER,
        GeometryShader = GL_GEOMETRY_SHADER
    };

    struct ShaderSource
    {
        std::string source;
        ShaderSourceType type;
    };

    using ShaderMemberHandle = GLint;

    bool CompileGLShader(ResourceHandle &outShader, const ShaderSourceType type, const char *source);
    bool LinkGLShaders(ResourceHandle &outShaderProgram, const ResourceHandle *shaders, const std::size_t shadersCount);
    bool CreateGLShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources, const std::size_t sourcesCount);

    inline constexpr const char *ShaderSourceTypeToString(ShaderSourceType type)
    {
        switch (type)
        {
            case ShaderSourceType::VertexShader: return "vertex";
            case ShaderSourceType::FragmentShader: return "fragment";
            case ShaderSourceType::GeometryShader: return "geometry";
            default: return "unknown";
        }
    }

    inline constexpr ShaderSourceType ShaderSourceTypeFromString(std::string_view stage)
    {
        if (stage == ".vert" || stage == ".vs" || stage == ".vertex" ||
            stage == "vertex" || stage == "vert" || stage == "vs")
            return ShaderSourceType::VertexShader;
        if (stage == ".frag" || stage == ".fs" || stage == ".fragment" ||
            stage == "fragment" || stage == "frag" || stage == "fs")
            return ShaderSourceType::FragmentShader;
        if (stage == ".geom" || stage == ".gs" || stage == ".geometry" ||
            stage == "geometry" || stage == "geom" || stage == "gs")
            return ShaderSourceType::GeometryShader;
        return ShaderSourceType::Unknown;
    }


    class Shader : public ResourceOwner
    {
    public:
        enum class MemberType
        {
            Uniform     = 0,
            Attribute   = 1
        };

    private:
        ResourceHandle shaderProgram = 0;
        std::vector<ShaderSource> sources;
        std::unordered_map<std::string, ShaderMemberHandle> memberLocations;

    public:
        [[nodiscard]] ResourceHandle GetProgramHandle() const noexcept                  { return shaderProgram; }
        [[nodiscard]] std::size_t GetSourceCount() const noexcept                       { return sources.size(); }
        [[nodiscard]] const ShaderSource &GetSource(std::size_t index) const noexcept   { return sources.at(index); }
        [[nodiscard]] const std::vector<ShaderSource> &GetSources() const noexcept      { return sources; }
        [[nodiscard]] bool HasSources() const noexcept                                  { return !sources.empty(); }
        [[nodiscard]] bool IsInitialized() const noexcept override                      { return shaderProgram != 0; }

        [[nodiscard]]
        bool GetUniformHandle(const std::string &name, ShaderMemberHandle &outHandle);
        [[nodiscard]]
        bool GetAttributeHandle(const std::string &name, ShaderMemberHandle &outHandle) const;

        template <typename T>
        bool SetUniformValue(const std::string &name, const T &value)
        {
            if constexpr (std::is_same_v<T, Int32>)
                return SetUniform(name, value, glUniform1i);
            else if constexpr (std::is_same_v<T, float>)
                return SetUniform(name, value, glUniform1f);
            else if constexpr (std::is_same_v<T, Vector2f>)
                return SetUniform(name, value, glUniform2fv);
            else if constexpr (std::is_same_v<T, Vector3f>)
                return SetUniform(name, value, glUniform3fv);
            else if constexpr (std::is_same_v<T, Vector4f>)
                return SetUniform(name, value, glUniform4fv);
            else if constexpr (std::is_same_v<T, Matrix4f>)
                return SetUniform(name, value, glUniformMatrix4fv);

            return false;
        }

        explicit Shader(std::vector<ShaderSource> sources);
        ~Shader() { ReleaseResources(); }

        Shader(Shader &&other) noexcept;
        Shader &operator=(Shader &&other) noexcept;
        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;

        void Use() const { glUseProgram(shaderProgram); }
        bool InitializeResources() override;

    private:
        template <typename T, typename UniformFunction>
        bool SetUniform(const std::string &name, const T &value, UniformFunction function)
        {
            ShaderMemberHandle location;
            if (!GetUniformHandle(name, location))
                return false;

            if constexpr (std::is_same_v<T, Matrix4f>)
                function(location, 1, GL_FALSE, glm::value_ptr(value));
            else if constexpr (std::is_integral_v<T>)
                function(location, value);
            else if constexpr (std::is_floating_point_v<T>)
                function(location, value);
            else
                function(location, 1, glm::value_ptr(value));

            return true;
        }

        void ReleaseResources() noexcept override;
    };
}