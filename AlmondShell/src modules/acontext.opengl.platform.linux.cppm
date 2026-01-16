// aopenglplatform_linux.cppm
module;

// Keep platform macros/types in the global module fragment.
#include "..\\include\\aengine.config.hpp"
#include "..\\include\\aengine.hpp"

#if defined(ALMOND_USING_OPENGL) && defined(__linux__)
#   include <X11/Xlib.h>
#   include <GL/glx.h>
#   include <dlfcn.h>
#   include <cstdint>
#endif

export module acontext.opengl.platform.linux;

import aengine.platform;
import acontext.opengl.platform;

#if defined(ALMOND_USING_OPENGL) && defined(__linux__)

namespace almondnamespace::openglcontext::PlatformGL
{
    PlatformGLContext get_current() noexcept
    {
        PlatformGLContext ctx{};
        ctx.display = ::glXGetCurrentDisplay();
        ctx.drawable = ::glXGetCurrentDrawable();
        ctx.context = ::glXGetCurrentContext();
        return ctx;
    }

    bool make_current(const PlatformGLContext& ctx) noexcept
    {
        // Match Win behavior: if ctx is invalid, clear current.
        if (!ctx.valid())
        {
            if (auto* dpy = ::glXGetCurrentDisplay())
                return ::glXMakeCurrent(dpy, 0, nullptr) == True;
            return true;
        }

        return ::glXMakeCurrent(ctx.display, ctx.drawable, ctx.context) == True;
    }

    void clear_current() noexcept
    {
        if (auto* dpy = ::glXGetCurrentDisplay())
            (void)::glXMakeCurrent(dpy, 0, nullptr);
    }

    void swap_buffers(const PlatformGLContext& ctx) noexcept
    {
        if (ctx.display && ctx.drawable)
            ::glXSwapBuffers(ctx.display, ctx.drawable);
    }

    void* get_proc_address(const char* name) noexcept
    {
        if (!name) return nullptr;

        if (auto p = ::glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(name)))
            return reinterpret_cast<void*>(p);

        // Fallback: try dlsym from libGL.so.1
        static void* libGL = ::dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
        return libGL ? ::dlsym(libGL, name) : nullptr;
    }
}

#endif // ALMOND_USING_OPENGL && __linux__
