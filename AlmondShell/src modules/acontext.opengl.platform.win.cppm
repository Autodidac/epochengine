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
 // aopenglplatform_win.cppm
module;

#include "..\\include\\aengine.config.hpp"
#include "..\\include\\aengine.hpp"

#if defined(ALMOND_USING_OPENGL) && defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <glad/glad.h>
#   include <GL/wglext.h>
#endif

export module acontext.opengl.platform.win;

import aengine.platform;
import acontext.opengl.platform;

#if defined(ALMOND_USING_OPENGL) && defined(_WIN32)

namespace almondnamespace::openglcontext::PlatformGL
{
    PlatformGLContext get_current() noexcept
    {
        PlatformGLContext ctx{};
        ctx.device = ::wglGetCurrentDC();
        ctx.context = ::wglGetCurrentContext();
        return ctx;
    }

    bool make_current(const PlatformGLContext& ctx) noexcept
    {
        if (!ctx.valid())
            return ::wglMakeCurrent(nullptr, nullptr) == TRUE;

        return ::wglMakeCurrent(ctx.device, ctx.context) == TRUE;
    }

    void clear_current() noexcept
    {
        (void)::wglMakeCurrent(nullptr, nullptr);
    }

    void swap_buffers(const PlatformGLContext& ctx) noexcept
    {
        if (ctx.device)
            (void)::SwapBuffers(ctx.device);
    }

    void* get_proc_address(const char* name) noexcept
    {
        if (!name) return nullptr;

        // wglGetProcAddress (extensions)
        if (auto p = reinterpret_cast<void*>(::wglGetProcAddress(name)))
        {
            // Filter out sentinel failure values some drivers return.
            const auto v = reinterpret_cast<std::uintptr_t>(p);
            if (v != 0 && v != 1 && v != 2 && v != 3 && v != static_cast<std::uintptr_t>(-1))
                return p;
        }

        // opengl32.dll exports
        static HMODULE opengl32 = ::GetModuleHandleW(L"opengl32.dll");
        if (!opengl32) opengl32 = ::LoadLibraryW(L"opengl32.dll");
        if (!opengl32) return nullptr;

        return reinterpret_cast<void*>(::GetProcAddress(opengl32, name));
    }
}

#endif // ALMOND_USING_OPENGL && _WIN32
