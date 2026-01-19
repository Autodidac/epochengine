/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝
 *
 *   This file is part of the Almond Project.
 *   AlmondShell - Modular C++ Framework
 *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell
 **************************************************************/

module;

#include <include/aengine.config.hpp>

#if defined(ALMOND_USING_RAYLIB)

// If this trips, something dragged Win32 headers into this TU.
// Fix the include graph: do NOT include windows.h (directly or indirectly)
// in anything reachable from this module.
#   if defined(_WIN32) && (defined(_WINDOWS_) || defined(_INC_WINDOWS) || defined(_WINUSER_))
#       error "acontext.raylib.api.cpp must not see <windows.h> before <raylib.h>. Move Win32 includes out of aengine.config.hpp / central headers."
#   endif

// Do NOT define RAYLIB_STATIC unless you are actually linking a static raylib build.
// For vcpkg shared builds, leave it undefined.
#   include <raylib.h>

#endif // ALMOND_USING_RAYLIB

export module acontext.raylib.api;

#if defined(ALMOND_USING_RAYLIB)

export namespace almondnamespace::raylib_api
{
    export using ::Color;
    export using ::Vector2;
    export using ::Texture2D;
    export using ::Image;
    export using ::Rectangle;

    export inline constexpr ::Color raywhite = ::RAYWHITE;
    export inline constexpr ::Color white = ::WHITE;

    export inline constexpr unsigned int flag_msaa_4x_hint = ::FLAG_MSAA_4X_HINT;
    export inline constexpr unsigned int flag_vsync_hint = ::FLAG_VSYNC_HINT;

    export inline constexpr int key_a = ::KEY_A;
    export inline constexpr int key_b = ::KEY_B;
    export inline constexpr int key_c = ::KEY_C;
    export inline constexpr int key_d = ::KEY_D;
    export inline constexpr int key_e = ::KEY_E;
    export inline constexpr int key_f = ::KEY_F;
    export inline constexpr int key_g = ::KEY_G;
    export inline constexpr int key_h = ::KEY_H;
    export inline constexpr int key_i = ::KEY_I;
    export inline constexpr int key_j = ::KEY_J;
    export inline constexpr int key_k = ::KEY_K;
    export inline constexpr int key_l = ::KEY_L;
    export inline constexpr int key_m = ::KEY_M;
    export inline constexpr int key_n = ::KEY_N;
    export inline constexpr int key_o = ::KEY_O;
    export inline constexpr int key_p = ::KEY_P;
    export inline constexpr int key_q = ::KEY_Q;
    export inline constexpr int key_r = ::KEY_R;
    export inline constexpr int key_s = ::KEY_S;
    export inline constexpr int key_t = ::KEY_T;
    export inline constexpr int key_u = ::KEY_U;
    export inline constexpr int key_v = ::KEY_V;
    export inline constexpr int key_w = ::KEY_W;
    export inline constexpr int key_x = ::KEY_X;
    export inline constexpr int key_y = ::KEY_Y;
    export inline constexpr int key_z = ::KEY_Z;

    export inline constexpr int key_zero = ::KEY_ZERO;
    export inline constexpr int key_one = ::KEY_ONE;
    export inline constexpr int key_two = ::KEY_TWO;
    export inline constexpr int key_three = ::KEY_THREE;
    export inline constexpr int key_four = ::KEY_FOUR;
    export inline constexpr int key_five = ::KEY_FIVE;
    export inline constexpr int key_six = ::KEY_SIX;
    export inline constexpr int key_seven = ::KEY_SEVEN;
    export inline constexpr int key_eight = ::KEY_EIGHT;
    export inline constexpr int key_nine = ::KEY_NINE;

    export inline constexpr int key_space = ::KEY_SPACE;
    export inline constexpr int key_enter = ::KEY_ENTER;
    export inline constexpr int key_escape = ::KEY_ESCAPE;
    export inline constexpr int key_tab = ::KEY_TAB;
    export inline constexpr int key_backspace = ::KEY_BACKSPACE;

    export inline constexpr int key_left = ::KEY_LEFT;
    export inline constexpr int key_right = ::KEY_RIGHT;
    export inline constexpr int key_up = ::KEY_UP;
    export inline constexpr int key_down = ::KEY_DOWN;

    export inline constexpr int mouse_button_left = ::MOUSE_BUTTON_LEFT;
    export inline constexpr int mouse_button_right = ::MOUSE_BUTTON_RIGHT;
    export inline constexpr int mouse_button_middle = ::MOUSE_BUTTON_MIDDLE;
    export inline constexpr int mouse_button_side = ::MOUSE_BUTTON_SIDE;
    export inline constexpr int mouse_button_extra = ::MOUSE_BUTTON_EXTRA;

    export inline constexpr int pixelformat_rgba8 = ::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    export inline void set_config_flags(unsigned int flags) { ::SetConfigFlags(flags); }
    export inline void init_window(int w, int h, const char* title) { ::InitWindow(w, h, title); }
    export inline void close_window() { ::CloseWindow(); }

    export inline bool window_should_close() { return ::WindowShouldClose(); }
    export inline bool is_window_ready() { return ::IsWindowReady(); }
    export inline void* get_window_handle() { return ::GetWindowHandle(); }

    export inline int get_render_width() { return ::GetRenderWidth(); }
    export inline int get_render_height() { return ::GetRenderHeight(); }
    export inline int get_screen_width() { return ::GetScreenWidth(); }
    export inline int get_screen_height() { return ::GetScreenHeight(); }

    export inline void begin_drawing() { ::BeginDrawing(); }
    export inline void end_drawing() { ::EndDrawing(); }
    export inline void clear_background(::Color c) { ::ClearBackground(c); }

    export inline void set_target_fps(int fps) { ::SetTargetFPS(fps); }
    export inline void set_window_title(const char* t) { ::SetWindowTitle(t); }

    export inline bool  is_key_down(int k) { return ::IsKeyDown(k); }
    export inline bool  is_mouse_button_down(int b) { return ::IsMouseButtonDown(b); }
    export inline int   get_mouse_x() { return ::GetMouseX(); }
    export inline int   get_mouse_y() { return ::GetMouseY(); }
    export inline float get_mouse_wheel_move() { return ::GetMouseWheelMove(); }
    export inline ::Vector2 get_mouse_position() { return ::GetMousePosition(); }
    export inline void set_mouse_offset(int ox, int oy) { ::SetMouseOffset(ox, oy); }
    export inline void set_mouse_scale(float sx, float sy) { ::SetMouseScale(sx, sy); }

    export inline ::Texture2D load_texture_from_image(const ::Image& img) { return ::LoadTextureFromImage(img); }
    export inline void unload_texture(const ::Texture2D& tex) { ::UnloadTexture(tex); }

    export inline void draw_texture_pro(
        const ::Texture2D& tex,
        const ::Rectangle& src,
        const ::Rectangle& dst,
        ::Vector2 origin,
        float rotation,
        ::Color tint)
    {
        ::DrawTexturePro(tex, src, dst, origin, rotation, tint);
    }
}

#endif // ALMOND_USING_RAYLIB
