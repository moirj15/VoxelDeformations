#include "utils.h"
#include "glm/vec3.hpp"
#include "glm/gtx/compatibility.hpp"

#include <SDL2/SDL.h>
#include <focus.hpp>
#include <glm/vec2.hpp>
#include <unordered_set>
#include <variant>
#include <memory>

/*
 * I'm thinking I'll make an input queue and just throw any inputs into there.
 *
 * Then various systems will opt in to certain events and ignore the rest
 */

enum class EventType {
    KeyPress,
    KeyChord,
    MouseMove,
    MouseDrag,
    MouseClick,
    Quit,
};

struct Event {
    const EventType type;

    explicit Event(EventType type) : type(type) {}

    virtual ~Event() = default;
};

struct KeyPressEvent : public Event {
    const u32 key;

    explicit KeyPressEvent(u32 key) : Event(EventType::KeyPress), key(key) {}
};

struct KeyChordEvent : public Event {
    const std::unordered_set<u8> keys;
    explicit KeyChordEvent(std::unordered_set<u8> &keys) : Event(EventType::KeyChord), keys(std::move(keys)) {}
};

struct MouseMoveEvent : public Event {
    const glm::ivec2 start_pos;
    const glm::ivec2 end_pos;

    MouseMoveEvent(const glm::ivec2 &start, const glm::ivec2 &end) :
            Event(EventType::MouseMove), start_pos(start), end_pos(end)
    {
    }
};

struct MouseDragEvent : public Event {
    const glm::ivec2 start_pos;
    const glm::ivec2 end_pos;
    const u32 button;

    MouseDragEvent(const glm::ivec2 &start, const glm::ivec2 &end, const u32 button) :
            Event(EventType::MouseDrag), start_pos(start), end_pos(end), button(button)
    {
    }
};

struct MouseClickEvent : public Event {
    const u32 button;
    explicit MouseClickEvent(const u32 button) : Event(EventType::MouseClick), button(button) {}
};

// using Event = std::variant<KeyPressEvent, KeyChordEvent, MouseMoveEvent, MouseDragEvent, MouseClickEvent>;

class EventQueue
{
    std::vector<std::unique_ptr<Event>> _events;

  public:
    void Push(Event *event) { _events.emplace_back(event); }
};

class InputManager
{
    EventQueue *_to_systems;

    glm::ivec2 _last_mouse_pos;
  public:
    void ConsumeInput()
    {
        SDL_Event e;
        std::unordered_set<u8> keys;
        while (SDL_PollEvent(&e) > 0) {
            if (e.type == SDL_QUIT) {
                _to_systems->Push(new Event(EventType::Quit));
            } else if (e.type == SDL_KEYDOWN && e.key.type == SDL_PRESSED) {
                keys.insert(e.key.keysym.scancode);
            } else if (e.type == SDL_MOUSEMOTION) {
                glm::ivec2 new_mouse_pos(e.motion.x, e.motion.y);
                _to_systems->Push(new MouseMoveEvent(_last_mouse_pos, new_mouse_pos));
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                glm::ivec2 new_mouse_pos(e.button.x, e.button.y);
                if (_last_mouse_pos == new_mouse_pos) {
                    _to_systems->Push(new MouseClickEvent(e.button.button));
                } else {
                    _to_systems->Push(new MouseDragEvent(_last_mouse_pos, new_mouse_pos, e.button.button));
                    _last_mouse_pos = new_mouse_pos;
                }
            }
        }
        if (keys.size() > 1) {
            _to_systems->Push(new KeyChordEvent(keys));
        } else {
            _to_systems->Push(new KeyPressEvent(*keys.begin()));
        }
    }
    void SubscribeToKeyPress(u8 key, u32 channel);
    void SubscribeToChord(const std::unordered_set<u8> &key_chord);
    void SubscribeToMouseClick(u32 channel);
    void SubscribeToMouseMove(u32 channel);
    void SubscribeToMouseDrag(u32 channel);
};

void ConsumeInput()
{
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0) {
        if (e.type == SDL_QUIT) {
        }
    }
}

class System {
  public:
    virtual ~System() = default;

    virtual void ReadEventQueue() = 0;
    virtual void RunFrame() = 0;
};

class UiSystem final : public System {
  public:
    void ReadEventQueue() override {}
    void RunFrame() override {}
};

class RenderSystem final : public System {
  public:
    void ReadEventQueue() override {}
    void RunFrame() override {}
};

glm::vec3 QuadraticBezier(const f32 t, const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2)
{
    return glm::mix(glm::mix(p0, p1, t), glm::mix(p1, p2, t), t);
}

void RunSystems()
{
}

static focus::Device *device = nullptr;
static focus::SceneState scene_state;
static focus::Pipeline line_pipeline;
static focus::Window window;

void RenderFrame()
{
    device->ClearBackBuffer({});
    device->BeginPass("Triangle pass");

    device->BindSceneState(scene_state);
    device->BindPipeline(line_pipeline);
    device->Draw(focus::Primitive::Lines, 0, 2);

    device->EndPass();

    device->SwapBuffers(window);
}

constexpr u32 point_size = 10;

int main(int argc, char **argv)
{
    device = focus::Device::Init(focus::RendererAPI::OpenGL);
    window = device->MakeWindow(1920, 1080);

    auto line_shader = device->CreateShaderFromSource("line_shader", utils::ReadEntireFileAsString("shaders/line.vert"),
        utils::ReadEntireFileAsString("shaders/line.frag"));

    focus::PipelineState pipeline_state = {
        .shader = line_shader,
        .line_width = 5.0f,
    };

    line_pipeline = device->CreatePipeline(pipeline_state);

    float line[] = {-1.0, -1.0, 1.0, 1.0};

    // clang-format off
    float mvp[] = {
        0.0f, 0.0f, 1.0f, 0.0f, // color + padding float
        // mvp matrix
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    // clang-format on

    focus::VertexBufferLayout vb_layout("Input");
    vb_layout.Add("aPosition", focus::VarType::Float2);

    focus::ConstantBufferLayout cb_layout("Constants");
    cb_layout.Add("color and mvp", focus::VarType::Float4x4);

    scene_state = {
        .vb_handles = {device->CreateVertexBuffer(vb_layout, line, sizeof(line))},
        .cb_handles = {device->CreateConstantBuffer(cb_layout, mvp, sizeof(mvp))},
    };

    InputManager input_manager;
    while (true) {
        input_manager.ConsumeInput();
        RunSystems();
        RenderFrame();
    }
}
