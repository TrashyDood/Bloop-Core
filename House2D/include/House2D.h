#pragma once

#include <glad/gl.h>

class House2D
{
public:
    explicit House2D(GLuint shader_program);

    void Draw() const;
    void HandleKey(int key, int action);

private:
    struct Color
    {
        float r;
        float g;
        float b;
    };

    void DrawTriangle(float x, float y, float width, float height, Color color) const;
    void DrawRectangle(float x, float y, float width, float height, Color color) const;

    GLint position_uniform_;
    GLint scale_uniform_;
    GLint color_uniform_;
    bool roof_color_changed_ = false;
};
