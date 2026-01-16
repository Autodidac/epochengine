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
 *                                                            *
 *   Redistribution Allowed with This Notice and              *
 *   LICENSE file. No obligation to disclose modifications.   *
 *                                                            *
 *   See LICENSE file for full terms.                         *
 *                                                            *
 **************************************************************/
 // acontext.opengl.state.ixx  (was aopenglstate.hpp)

module;

// Keep platform macros/types and GL headers in the global module fragment.
#include "..\include\aengine.config.hpp"

#if defined(__linux__)
#    include <X11/Xlib.h>
#    include <GL/glx.h>
#endif

export module acontext.opengl.state;

import <array>;
import <bitset>;
import <functional>;

import aengine.platform;      // for HWND, HDC, HGLRC, Display, etc.
import aengine.core.time;        // time::Timer, time::createTimer(...)
import acontext.opengl.platform; // PlatformGLContext   

#if defined(ALMOND_USING_OPENGL)

export namespace almondnamespace::openglstate
{
    struct OpenGL4State
    {
#ifdef ALMOND_USING_WINMAIN
        HWND  hwnd{};
        HDC   hdc = nullptr;
        HGLRC hglrc = nullptr;

        HGLRC  glContext{}; // Store GL context created
        WNDPROC oldWndProc = nullptr;
        WNDPROC getOldWndProc() const noexcept { return oldWndProc; }

        HWND parent = nullptr;
        std::function<void(int, int)> onResize;
        unsigned int width{ 400 };
        unsigned int height{ 300 };

        bool running{ false };

#elif defined(__linux__)
        ::Window   window = 0;
        GLXDrawable drawable = 0;
        Display* display = nullptr;
        GLXContext glxContext = nullptr;
        GLXFBConfig fbConfig = nullptr;
        Colormap   colormap = 0;

        bool ownsDisplay = false;
        bool ownsWindow = false;
        bool ownsContext = false;
        bool ownsColormap = false;

        unsigned int width{ 400 };
        unsigned int height{ 300 };
#endif

        struct MouseState
        {
            std::array<bool, 5> down{};
            std::array<bool, 5> pressed{};
            std::array<bool, 5> prevDown{};
            int lastX = 0;
            int lastY = 0;
        } mouse;

        struct KeyboardState
        {
            std::bitset<256> down;
            std::bitset<256> pressed;
            std::bitset<256> prevDown;
        } keyboard;

        timing::Timer pollTimer = timing::createTimer(1.0);
        timing::Timer fpsTimer = timing::createTimer(1.0);
        int frameCount = 0;

        GLuint shader = 0;
        GLint  uUVRegionLoc = -1;
        GLint  uTransformLoc = -1;
        GLint  uSamplerLoc = -1;
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLuint fbo = 0;
    };

    // Inline global instance, header-only style
    inline OpenGL4State s_openglstate{};
} // namespace almondnamespace::openglstate

#endif // ALMOND_USING_OPENGL
