/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝
 *
 *   AlmondShell – Raylib Context (Portable)
 **************************************************************/

module;

#include <include/aengine.config.hpp> // for ALMOND_USING Macros

export module acontext.raylib.context;

import <algorithm>;
import <cmath>;
import <cstdint>;
import <functional>;
import <memory>;
import <string>;
import <string_view>;
import <utility>;

import aengine.core.context;
import aengine.core.commandline;
import aatlas.manager;

import acontext.raylib.state;
import acontext.raylib.textures;
import acontext.raylib.renderer;
import acontext.raylib.input;
import acontext.raylib.api;

#if defined(ALMOND_USING_RAYLIB)

namespace almondnamespace::raylibcontext
{
    // ------------------------------------------------------------
    // Portable types
    // ------------------------------------------------------------
    using NativeWindowHandle = void*;

    inline std::string& title_storage()
    {
        static std::string s_title = "AlmondShell";
        return s_title;
    }

    // ------------------------------------------------------------
    // Initialization
    // ------------------------------------------------------------
    export inline bool raylib_initialize(
        std::shared_ptr<core::Context> ctx,
        NativeWindowHandle /*parent*/ = nullptr,
        unsigned width = 0,
        unsigned height = 0,
        std::function<void(int, int)> resizeCallback = nullptr,
        std::string title = {})
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;

        if (width == 0)  width = static_cast<unsigned>(core::cli::window_width);
        if (height == 0) height = static_cast<unsigned>(core::cli::window_height);

        st.width = (std::max)(1u, width);
        st.height = (std::max)(1u, height);
        st.onResize = std::move(resizeCallback);

        if (!title.empty())
            title_storage() = std::move(title);

        almondnamespace::raylib_api::set_config_flags(
            static_cast<unsigned>(almondnamespace::raylib_api::flag_msaa_4x_hint));

        almondnamespace::raylib_api::init_window(
            static_cast<int>(st.width),
            static_cast<int>(st.height),
            title_storage().c_str());

        almondnamespace::raylib_api::set_target_fps(0);

        if (ctx)
        {
            // The multiplexer expects ctx->hwnd to become the backend-created HWND.
            ctx->hwnd = static_cast<decltype(ctx->hwnd)>(almondnamespace::raylib_api::get_window_handle());

            // If you also keep native_window, keep it consistent (optional).
            if constexpr (requires { ctx->native_window; })
                ctx->native_window = almondnamespace::raylib_api::get_window_handle();
        }

#if defined(_WIN32)
        {
            // Keep raylibstate coherent too (so the Win32 glue module can rely on it).
            auto hwnd = static_cast<HWND>(almondnamespace::raylib_api::get_window_handle());
            st.hwnd = hwnd;
        }
#else
        {
            st.hwnd = almondnamespace::raylib_api::get_window_handle();
        }
#endif


        st.running = true;
        st.cleanupIssued = false;

        almondnamespace::atlasmanager::register_backend_uploader(
            almondnamespace::core::ContextType::RayLib,
            almondnamespace::raylibtextures::ensure_uploaded);

        return true;
    }

    // ------------------------------------------------------------
    // Per-frame processing
    // ------------------------------------------------------------
    export inline void raylib_process()
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

        const int w = almondnamespace::raylib_api::get_screen_width();
        const int h = almondnamespace::raylib_api::get_screen_height();

        if (w > 0 && h > 0 &&
            (static_cast<unsigned>(w) != st.width ||
                static_cast<unsigned>(h) != st.height))
        {
            st.width = static_cast<unsigned>(w);
            st.height = static_cast<unsigned>(h);

            if (st.onResize)
                st.onResize(w, h);
        }
    }

    // ------------------------------------------------------------
    // Frame helpers
    // ------------------------------------------------------------
    export inline void raylib_clear(float r, float g, float b, float /*a*/)
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

        almondnamespace::raylib_api::begin_drawing();
        almondnamespace::raylib_api::clear_background(
            almondnamespace::raylib_api::Color{
                static_cast<unsigned char>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
                static_cast<unsigned char>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
                static_cast<unsigned char>(std::clamp(b, 0.0f, 1.0f) * 255.0f),
                255
            });
    }

    export inline void raylib_present()
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

        almondnamespace::raylib_api::end_drawing();
    }

    // ------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------
    export inline void raylib_cleanup(std::shared_ptr<core::Context>)
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running || st.cleanupIssued)
            return;

        st.cleanupIssued = true;

        almondnamespace::atlasmanager::unregister_backend_uploader(
            almondnamespace::core::ContextType::RayLib);

        almondnamespace::raylibtextures::shutdown_current_context_backend();
        almondnamespace::raylib_api::close_window();

        st = {};
    }

    // ------------------------------------------------------------
    // Utilities
    // ------------------------------------------------------------
    export inline void raylib_set_window_title(std::string_view title)
    {
        title_storage().assign(title.begin(), title.end());
        almondnamespace::raylib_api::set_window_title(title_storage().c_str());
    }

    export inline int raylib_get_width()
    {
        return static_cast<int>(almondnamespace::raylibstate::s_raylibstate.width);
    }

    export inline int raylib_get_height()
    {
        return static_cast<int>(almondnamespace::raylibstate::s_raylibstate.height);
    }

    export inline almondnamespace::raylib_api::Vector2 raylib_get_mouse_position()
    {
        return almondnamespace::raylib_api::get_mouse_position();
    }

    export inline NativeWindowHandle raylib_get_native_window()
    {
        return almondnamespace::raylib_api::get_window_handle();
    }
}

#endif // ALMOND_USING_RAYLIB









