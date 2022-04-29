#include "glm/gtx/compatibility.hpp"
#include "utils.h"

#include <SDL2/SDL.h>
#include <focus.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <optional>
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

struct DataManager {
    static constexpr f32 point_size = 10.0f;
    static constexpr s32 screen_width = 720;
    static constexpr s32 screen_height = 640;

    bool should_quit = false;
    glm::vec2 control_points[3] = {
        glm::vec2(-0.75, -0.75),
        glm::vec2(0, 0.75),
        glm::vec2(0.75, -0.75),
    };
    std::vector<glm::vec2> bezier_line_segments;
    //    glm::ivec2 mouse_pos = {screen_width / 2, screen_height / 2};
    std::optional<glm::ivec2> mouse_held_pos;
    std::optional<u32> clicked_point;
};

class System
{
  protected:
    DataManager *_data_manager;

  public:
    explicit System(DataManager *data_manager) : _data_manager(data_manager) {}
    virtual ~System() = default;
    virtual void Run() = 0;
};

class InputSystem : public System
{
  public:
    explicit InputSystem(DataManager *data_manager) : System(data_manager) {}
    void Run() override
    {
        SDL_Event e;
        while (SDL_PollEvent(&e) > 0) {
            if (e.type == SDL_QUIT) {
                _data_manager->should_quit = true;
                return;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                _data_manager->mouse_held_pos = {e.button.x, e.button.y};
            } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                _data_manager->mouse_held_pos.reset();
            } else if (e.type == SDL_MOUSEMOTION && _data_manager->mouse_held_pos) {
                _data_manager->mouse_held_pos = {e.motion.x, e.motion.y};
            }
        }
    }
};

// TODO: I can put these in their own .cpp files and only have a .h with a funcion for creating the system
/*

 system.h

 System *CreateSystem();
 end_system.h

 system.cpp

 class System { ... }
 */
class RenderSystem : public System
{
    focus::Device *_device = nullptr;
    focus::Window _window;

    focus::SceneState _line_scene_state;
    focus::Pipeline _line_pipeline;

    focus::SceneState point_scene_state;
    focus::Pipeline point_pipeline;

    focus::DynamicVertexBuffer _control_point_buffer;
    focus::DynamicVertexBuffer _line_buffer;

  public:
    explicit RenderSystem(DataManager *data_manager) : System(data_manager)
    {
        _device = focus::Device::Init(focus::RendererAPI::OpenGL);
        _window = _device->MakeWindow(DataManager::screen_width, DataManager::screen_height);

        //
        // Init line pipeline
        //

        auto line_shader = _device->CreateShaderFromSource("line_shader",
            utils::ReadEntireFileAsString("shaders/line.vert"), utils::ReadEntireFileAsString("shaders/line.frag"));

        // TODO: implement a line rendering pipeline
        focus::PipelineState line_pipeline_state = {
            .shader = line_shader,
            .line_width = 5.0f,
        };

        _line_pipeline = _device->CreatePipeline(line_pipeline_state);

        //    float line[] = {-1.0, -1.0, 1.0, 1.0};

        // clang-format off
        float line_mvp[] = {
            0.0f, 0.0f, 1.0f, 0.0f, // color + padding float
            // mvp matrix
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        // clang-format on

        focus::VertexBufferLayout line_vb_layout("Input");
        line_vb_layout.Add("aPosition", focus::VarType::Float2);

        focus::ConstantBufferLayout line_cb_layout("Constants");
        line_cb_layout.Add("color and mvp", focus::VarType::Float4x4);

        _line_buffer = _device->CreateDynamicVertexBuffer(line_vb_layout, _data_manager->bezier_line_segments.data(),
            _data_manager->bezier_line_segments.size() * sizeof(glm::vec2));
        _line_scene_state = {
            .dynamic_vb_handles = {_line_buffer},
            .cb_handles = {_device->CreateConstantBuffer(line_cb_layout, line_mvp, sizeof(line_mvp))},
        };

        //
        // Init point pipeline
        //

        auto point_shader = _device->CreateShaderFromSource("point_shader",
            utils::ReadEntireFileAsString("shaders/point.vert"), utils::ReadEntireFileAsString("shaders/point.frag"));

        focus::PipelineState point_pipeline_state = {
            .shader = point_shader,
        };

        point_pipeline = _device->CreatePipeline(point_pipeline_state);

        // clang-format off
        float point_mvp[] = {
            1.0f, 0.0f, 0.0f, // color + padding float
            DataManager::point_size, // point size
            // mvp matrix
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        // clang-format on

        focus::VertexBufferLayout point_vb_layout("Input");
        point_vb_layout.Add("aPosition", focus::VarType::Float2);

        focus::ConstantBufferLayout point_cb_layout("Constants");
        point_cb_layout.Add("color size and mvp", focus::VarType::Float4x4);

        _control_point_buffer = _device->CreateDynamicVertexBuffer(
            point_vb_layout, _data_manager->control_points, sizeof(_data_manager->control_points));
        point_scene_state = {
            .dynamic_vb_handles = {_control_point_buffer},
            .cb_handles = {_device->CreateConstantBuffer(point_cb_layout, point_mvp, sizeof(point_mvp))},
        };
    }

    void Run() override
    {
        _device->UpdateDynamicVertexBuffer(
            _control_point_buffer, _data_manager->control_points, sizeof(_data_manager->control_points));
        //        auto line_points = CreateBezierLines();
        _device->UpdateDynamicVertexBuffer(_line_buffer, _data_manager->bezier_line_segments.data(),
            _data_manager->bezier_line_segments.size() * sizeof(glm::vec2));

        _device->ClearBackBuffer({});
        _device->BeginPass("Line Pass");

        _device->BindSceneState(_line_scene_state);
        _device->BindPipeline(_line_pipeline);
        _device->Draw(focus::Primitive::LineStrip, 0, 100);

        _device->EndPass();

        _device->BeginPass("Point Pass");

        _device->BindSceneState(point_scene_state);
        _device->BindPipeline(point_pipeline);
        _device->Draw(focus::Primitive::Points, 0, 3);

        _device->EndPass();

        _device->SwapBuffers(_window);
    }
};

class PointSystem : public System
{
    std::optional<u32> _point_index;

  public:
    explicit PointSystem(DataManager *data_manager) : System(data_manager) {}
    void Run() override
    {
        if (!_data_manager->mouse_held_pos) {
            _point_index.reset();
            return;
        }
        const auto &mouse_pos = _data_manager->mouse_held_pos.value();
        if (!_point_index) {
            _point_index = PointHitByMouse(mouse_pos);
        }
        if (_point_index) {
            _data_manager->control_points[_point_index.value()] =
                utils::ScreenSpaceToNDC(mouse_pos, DataManager::screen_width, DataManager::screen_height);
        }
    }

  private:
    std::optional<u32> PointHitByMouse(const glm::ivec2 &mouse_pos)
    {
        for (u32 i = 0; i < 3; i++) {
            const auto &point = utils::NDCToScreenSpace(
                _data_manager->control_points[i], DataManager::screen_width, DataManager::screen_height);
            const auto lower_left = point - (static_cast<s32>(DataManager::point_size) / 2);
            const auto upper_right = point + (static_cast<s32>(DataManager::point_size) / 2);
            if (glm::all(glm::lessThanEqual(lower_left, mouse_pos))
                && glm::all(glm::greaterThanEqual(upper_right, mouse_pos))) {
                return i;
            }
        }
        return {};
    }
};

class LineSystem : public System
{
  public:
    explicit LineSystem(DataManager *data_manager) : System(data_manager)
    {
        _data_manager->bezier_line_segments = CreateBezierLines();
    }
    void Run() override { _data_manager->bezier_line_segments = CreateBezierLines(); }

    static glm::vec2 QuadraticBezier(const f32 t, const glm::vec2 &p0, const glm::vec2 &p1, const glm::vec2 &p2)
    {
        return glm::mix(glm::mix(p0, p1, t), glm::mix(p1, p2, t), t);
    }

    std::vector<glm::vec2> CreateBezierLines()
    {
        std::vector<glm::vec2> points;
        for (u32 i = 0; i < 100; i++) {
            points.emplace_back(QuadraticBezier(static_cast<f32>(i) / 100.0f, _data_manager->control_points[0],
                _data_manager->control_points[1], _data_manager->control_points[2]));
        }
        return points;
    }
};

class SystemManager : public System
{
    std::vector<std::unique_ptr<System>> _systems;

  public:
    explicit SystemManager(DataManager *data_manager) : System(data_manager)
    {
        _systems.emplace_back(new InputSystem(data_manager));
        _systems.emplace_back(new PointSystem(data_manager));
        _systems.emplace_back(new LineSystem(data_manager));
        _systems.emplace_back(new RenderSystem(data_manager));
    }
    void Run() override
    {
        while (!_data_manager->should_quit) {
            for (const auto &system : _systems) {
                system->Run();
            }
        }
    }
};

int main(int argc, char **argv)
{
    DataManager data_manager;
    SystemManager system_manager(&data_manager);
    system_manager.Run();

    return 0;
}
