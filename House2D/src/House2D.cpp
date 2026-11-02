#include "House2D.h"

#include <GLFW/glfw3.h>

House2D::House2D(GLuint shader_program)
    : position_uniform_(glGetUniformLocation(shader_program, "position")),
      scale_uniform_(glGetUniformLocation(shader_program, "scale")),
      color_uniform_(glGetUniformLocation(shader_program, "color")) {}

void House2D::DrawTriangle(float x, float y, float width, float height, Color color) const
{
    glUniform2f(position_uniform_, x, y); // Set the position uniform for the triangle.
    glUniform2f(scale_uniform_, width, height); // Set the scale uniform for the triangle.
    glUniform3f(color_uniform_, color.r, color.g, color.b); // Set the color uniform for the triangle.
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void House2D::DrawRectangle(float x, float y, float width, float height, Color color) const
{
    DrawTriangle(x, y, width, height, color);
    DrawTriangle(x + width, y + height, -width, -height, color);
}

void House2D::Draw() const
{
    constexpr Color wall{0.82f, 0.55f, 0.30f};
    constexpr Color roof{0.75f, 0.28f, 0.20f};
    constexpr Color changed_roof{0.20f, 0.45f, 0.85f};
    constexpr Color door{0.35f, 0.19f, 0.12f};
    constexpr Color window{0.35f, 0.75f, 0.88f};

    DrawRectangle(-0.55f, -0.75f, 1.10f, 0.87f, wall);
    DrawRectangle(-0.14f, -0.75f, 0.28f, 0.47f, door);
    DrawRectangle(-0.46f, -0.14f, 0.21f, 0.18f, window);
    DrawRectangle(0.25f, -0.14f, 0.21f, 0.18f, window);

    const Color roof_color = roof_color_changed_ ? changed_roof : roof;
    DrawTriangle(-0.70f, 0.08f, 0.70f, 0.68f, roof_color);
    DrawTriangle(0.70f, 0.08f, -0.70f, 0.68f, roof_color);
}

void House2D::HandleKey(int key, int action)
{
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        roof_color_changed_ = !roof_color_changed_;
}