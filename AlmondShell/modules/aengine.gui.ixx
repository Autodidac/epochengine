
export module aengine.gui;

import aengine.core.context;
import aspritehandle; // must export SpriteHandle (you have aspritehandle.ixx)

import <cstdint>;
import <memory>;
import <optional>;
import <string>;
import <string_view>;
import <vector>;

export namespace almondnamespace::gui
{
    // ---- Basic math / data types (were in agui.hpp) ----
    export struct Vec2
    {
        float x{};
        float y{};

        constexpr Vec2() = default;
        constexpr Vec2(float _x, float _y) : x(_x), y(_y) {}

        constexpr Vec2& operator+=(Vec2 rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
        friend constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { a += b; return a; }
    };

    struct Color
    {
        std::uint8_t r{}, g{}, b{}, a{ 255 };
    };

    enum class EventType : std::uint8_t
    {
        None = 0,
        MouseMove,
        MouseDown,
        MouseUp,
        KeyDown,
        KeyUp,
        TextInput
    };

    export struct InputEvent
    {
        EventType type{ EventType::None };
        Vec2 mouse_pos{};
        int key{};                 // platform-agnostic key code (legacy behavior)
        std::string text{};        // for TextInput
    };

    export struct WidgetBounds
    {
        Vec2 position{};
        Vec2 size{};
    };

    struct EditBoxResult
    {
        bool active{};
        bool changed{};
        bool submitted{};
    };

    struct ConsoleWindowOptions
    {
        std::string_view title{};
        Vec2 position{};
        Vec2 size{};

        const std::vector<std::string>& lines;
        std::size_t max_visible_lines{ 200 };

        std::string* input{ nullptr };
        std::size_t max_input_chars{ 4096 };
        bool multiline_input{ false };
    };

    struct ConsoleWindowResult
    {
        EditBoxResult input{};
    };

    // ---- Public API ----
    export void push_input(const InputEvent& e) noexcept;

    export void begin_frame(const std::shared_ptr<core::Context>& ctx,
        float dt,
        Vec2 mouse_pos,
        bool mouse_down) noexcept;

    export void end_frame() noexcept;

    export void begin_window(std::string_view title, Vec2 position, Vec2 size) noexcept;
    export void end_window() noexcept;

    export void set_cursor(Vec2 position) noexcept;
    export void advance_cursor(Vec2 delta) noexcept;

    export bool button(std::string_view label, Vec2 size) noexcept;
    export bool image_button(const SpriteHandle& sprite, Vec2 size) noexcept;

    export EditBoxResult edit_box(std::string& text,
        Vec2 size,
        std::size_t max_chars = 0,
        bool multiline = false) noexcept;

    export void text_box(std::string_view text, Vec2 size) noexcept;

    export ConsoleWindowResult console_window(const ConsoleWindowOptions& options) noexcept;

    export void label(std::string_view text) noexcept;

    export float line_height() noexcept;
    export float glyph_width() noexcept;

    export std::optional<WidgetBounds> last_button_bounds() noexcept;
}
