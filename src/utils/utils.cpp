#include "utils.h"
namespace utils
{
glm::vec2 ScreenSpaceToNDC(const glm::vec2 &mouse_pos, const int screen_width, const int screen_height)
{
    return {(mouse_pos.x - (static_cast<f32>(screen_width) / 2.0f)) / (static_cast<f32>(screen_width) / 2.0f),
            ((static_cast<f32>(screen_height) - mouse_pos.y) - (static_cast<f32>(screen_height) / 2.0f))
            / (static_cast<f32>(screen_height) / 2.0f)};
}

glm::ivec2 NDCToScreenSpace(const glm::vec2 &screen_space, const int screen_width, const int screen_height)
{
    return {(screen_space.x * (static_cast<f32>(screen_width) / 2.0f)) + (static_cast<f32>(screen_width) / 2.0f),
            (-screen_space.y * (static_cast<f32>(screen_height) / 2.0f)) + (static_cast<f32>(screen_height) / 2.0f)};
}

} // namespace utils
