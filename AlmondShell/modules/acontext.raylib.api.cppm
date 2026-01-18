/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 *                                                            *
 *   This file is part of the Almond Project.                 *
 *   AlmondShell - Modular C++ Framework                      *
 *                                                            *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
 *                                                            *
 *   Provided "AS IS", without warranty of any kind.          *
 *   Use permitted for Non-Commercial Purposes ONLY,          *
 *   without prior commercial licensing agreement.            *
 **************************************************************/

module;

#include "aengine.config.hpp"

#if defined(ALMOND_USING_RAYLIB)
#include <raylib.h>
#endif

export module acontext.raylib.api;

#if defined(ALMOND_USING_RAYLIB)

export namespace almondnamespace::raylib_api
{
    using ::Color;
    using ::Vector2;

    export inline constexpr int flag_msaa_4x_hint = FLAG_MSAA_4X_HINT;

    export inline void init_window(int width, int height, const char* title)
    {
        ::InitWindow(width, height, title);
    }

    export inline void close_window()
    {
        ::CloseWindow();
    }

    export inline bool window_should_close()
    {
        return ::WindowShouldClose();
    }

    export inline bool is_window_ready()
    {
        return ::IsWindowReady();
    }

    export inline void* get_window_handle()
    {
        return ::GetWindowHandle();
    }

    export inline int get_render_width()
    {
        return ::GetRenderWidth();
    }

    export inline int get_render_height()
    {
        return ::GetRenderHeight();
    }

    export inline int get_screen_width()
    {
        return ::GetScreenWidth();
    }

    export inline int get_screen_height()
    {
        return ::GetScreenHeight();
    }

    export inline void begin_drawing()
    {
        ::BeginDrawing();
    }

    export inline void end_drawing()
    {
        ::EndDrawing();
    }

    export inline void clear_background(Color color)
    {
        ::ClearBackground(color);
    }

    export inline void set_config_flags(unsigned int flags)
    {
        ::SetConfigFlags(flags);
    }

    export inline void set_target_fps(int fps)
    {
        ::SetTargetFPS(fps);
    }

    export inline void set_window_title(const char* title)
    {
        ::SetWindowTitle(title);
    }

    export inline Vector2 get_mouse_position()
    {
        return ::GetMousePosition();
    }

    export inline Vector2 get_render_offset()
    {
        return ::GetRenderOffset();
    }

    export inline void set_mouse_offset(int offsetX, int offsetY)
    {
        ::SetMouseOffset(offsetX, offsetY);
    }

    export inline void set_mouse_scale(float scaleX, float scaleY)
    {
        ::SetMouseScale(scaleX, scaleY);
    }
}

#endif // ALMOND_USING_RAYLIB
