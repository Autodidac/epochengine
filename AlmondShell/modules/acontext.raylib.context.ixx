module;

#if defined(_WIN32)
#include <windows.h>
#include <gl/gl.h>
#endif

export module acontext.raylib.context;

import <algorithm>;
import <cassert>;
import <cmath>;
import <cstdint>;
import <exception>;
import <functional>;
import <iostream>;
import <memory>;
import <string>;
import <string_view>;
import <utility>;
import <vector>;
import <filesystem>;


// Raylib as a header-unit import (requires header-units enabled in your toolchain)
import <raylib.h>;

// ------------------------------------------------------------
// Core platform + engine modules (C++23 imports)
// ------------------------------------------------------------
import aengine.platform;                 // replaces aplatform.hpp
import aengine.config;            // replaces aengineconfig.hpp
import aengine.context;           // replaces acontext.hpp
import aengine.context.window;    // replaces awindowdata.hpp / acontextwindow.hpp
import aengine.core.time;        // already used in your header
import aimageloader;              // replaces aimageloader.hpp
import aatlas.manager;            // replaces aatlasmanager.hpp
import aengine.core.commandline;              // replaces acommandline.hpp
import aengine.input;             // replaces ainput.hpp

// ------------------------------------------------------------
// Raylib backend peer modules
// ------------------------------------------------------------
import acontext.raylib.textures;          // replaces araylibtextures.hpp
import acontext.raylib.state;             // replaces araylibstate.hpp
import acontext.raylib.renderer;          // replaces araylibrenderer.hpp
import acontext.raylib.input;     // replaces araylibcontextinput.hpp


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

#if defined(ALMOND_USING_RAYLIB)

#if defined(_WIN32)
namespace almondnamespace::core { void MakeDockable(HWND hwnd, HWND parent); }
#endif

namespace almondnamespace::raylibcontext
{
#if defined(_WIN32)
    using NativeWindowHandle = HWND;
    using NativeDeviceContext = HDC;
    using NativeGlContext = HGLRC;
#else
    using NativeWindowHandle = void*;
    using NativeDeviceContext = void*;
    using NativeGlContext = void*;
#endif

    // ------------------------------------------------------------
    // Internal WGL helpers (Windows only)
    // ------------------------------------------------------------
#if defined(_WIN32)

    struct wgl_handles
    {
        NativeWindowHandle hwnd{};
        NativeDeviceContext hdc{};
        NativeGlContext glContext{};
    };

    inline wgl_handles& state() noexcept
    {
        // Provided by araylibstate.hpp/module (expects inline storage there)
        return s_raylibstate.wgl;
    }

    inline NativeWindowHandle GetWindowHandle()
    {
        // raylib exposes this on Windows; keep as-is from your original logic
        return static_cast<NativeWindowHandle>(::GetWindowHandle());
    }

    inline bool make_current(NativeDeviceContext dc, NativeGlContext rc) noexcept
    {
        return ::wglMakeCurrent(dc, rc) == TRUE;
    }

    inline void clear_current() noexcept
    {
        ::wglMakeCurrent(nullptr, nullptr);
    }

    inline bool refresh_wgl_handles()
    {
        const auto currentWindow = static_cast<HWND>(GetWindowHandle());
        const bool windowChanged = currentWindow && currentWindow != s_raylibstate.hwnd;

        if (!windowChanged && s_raylibstate.hdc && s_raylibstate.glContext) {
            if (make_current(s_raylibstate.hdc, s_raylibstate.glContext)) {
                return true;
            }
        }

        const HWND previousWindow = s_raylibstate.hwnd;
        const HDC previousDC = s_raylibstate.hdc;
        const HGLRC previousRC = s_raylibstate.glContext;

        s_raylibstate.hwnd = currentWindow;
        s_raylibstate.hdc = currentWindow ? ::GetDC(currentWindow) : nullptr;
        s_raylibstate.glContext = ::wglGetCurrentContext();

        if (!s_raylibstate.hwnd || !s_raylibstate.hdc || !s_raylibstate.glContext)
        {
            s_raylibstate.hwnd = previousWindow;
            s_raylibstate.hdc = previousDC;
            s_raylibstate.glContext = previousRC;
            return false;
        }

        if (!make_current(s_raylibstate.hdc, s_raylibstate.glContext))
        {
            s_raylibstate.hwnd = previousWindow;
            s_raylibstate.hdc = previousDC;
            s_raylibstate.glContext = previousRC;
            return false;
        }

        return true;
    }

#endif // _WIN32

    // ------------------------------------------------------------
    // Public API (kept identical to your original header intent)
    // ------------------------------------------------------------

    inline bool raylib_initialize(
        std::shared_ptr<Context> ctx,
        NativeWindowHandle parent = nullptr,
        unsigned width = 0,
        unsigned height = 0,
        std::function<void(int, int)> resizeCallback = nullptr,
        std::string title = {})
    {
        (void)ctx;

        if (width == 0)  width = static_cast<unsigned>(cli::window_width);
        if (height == 0) height = static_cast<unsigned>(cli::window_height);

        s_raylibstate.parent = parent;
        s_raylibstate.width = static_cast<int>(width);
        s_raylibstate.height = static_cast<int>(height);
        s_raylibstate.resize_callback = std::move(resizeCallback);

        if (!title.empty())
            s_raylibstate.title = std::move(title);

        // Let raylib create window/context
        ::SetConfigFlags(FLAG_MSAA_4X_HINT);
        ::InitWindow(s_raylibstate.width, s_raylibstate.height, s_raylibstate.title.c_str());
        ::SetTargetFPS(0);

#if defined(_WIN32)
        if (s_raylibstate.parent)
        {
            const HWND hwnd = static_cast<HWND>(GetWindowHandle());
            const HWND parentHwnd = static_cast<HWND>(s_raylibstate.parent);

            // Optional: docking / parenting
            almondnamespace::core::MakeDockable(hwnd, parentHwnd);
        }

        // Capture WGL handles (for restoring current context on multi-context engines)
        s_raylibstate.hwnd = static_cast<HWND>(GetWindowHandle());
        s_raylibstate.hdc = s_raylibstate.hwnd ? ::GetDC(s_raylibstate.hwnd) : nullptr;
        s_raylibstate.glContext = ::wglGetCurrentContext();
#endif

        // Initialize renderer layer
        raylibrenderer::initialize();

        // Initialize input bridge (if any)
        raylibcontextinput::initialize();

        s_raylibstate.initialized = true;
        return true;
    }

    inline void raylib_process()
    {
        if (!s_raylibstate.initialized) return;

#if defined(_WIN32)
        (void)refresh_wgl_handles();
#endif

        raylibcontextinput::process();

        // Handle window resize -> callback
        const int w = ::GetScreenWidth();
        const int h = ::GetScreenHeight();
        if (w != s_raylibstate.width || h != s_raylibstate.height)
        {
            s_raylibstate.width = w;
            s_raylibstate.height = h;

            if (s_raylibstate.resize_callback)
                s_raylibstate.resize_callback(w, h);
        }
    }

    inline void raylib_clear(float r, float g, float b, float a)
    {
        (void)a;
        if (!s_raylibstate.initialized) return;

        ::BeginDrawing();
        ::ClearBackground(::Color{
            static_cast<unsigned char>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
            static_cast<unsigned char>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
            static_cast<unsigned char>(std::clamp(b, 0.0f, 1.0f) * 255.0f),
            255
            });
    }

    inline void raylib_present()
    {
        if (!s_raylibstate.initialized) return;

        raylibrenderer::present();

        ::EndDrawing();
    }

    inline void raylib_cleanup(std::shared_ptr<Context> ctx)
    {
        (void)ctx;
        if (!s_raylibstate.initialized) return;

        raylibrenderer::shutdown();
        raylibcontextinput::shutdown();

        ::CloseWindow();

        s_raylibstate = {}; // reset state
    }

    inline void raylib_set_window_title(std::string_view title)
    {
        s_raylibstate.title.assign(title.begin(), title.end());
        ::SetWindowTitle(s_raylibstate.title.c_str());
    }

    inline int raylib_get_width()
    {
        return s_raylibstate.width;
    }

    inline int raylib_get_height()
    {
        return s_raylibstate.height;
    }

    inline ::Vector2 raylib_get_mouse_position()
    {
        return ::GetMousePosition();
    }
} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
