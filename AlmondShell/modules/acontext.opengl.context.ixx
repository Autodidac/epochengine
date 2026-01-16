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
// acontext.opengl.context.ixx
module;

// OS + GL headers in global module fragment.
#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <glad/glad.h>
#   include <GL/wglext.h>
#elif defined(__linux__)
#   include <glad/glad.h>
#   include <X11/Xlib.h>
#   include <X11/Xutil.h>
#   include <GL/glx.h>
#   include <GL/glxext.h>
#endif

#include "..\\include\\aengine.config.hpp"

export module acontext.opengl.context;

// ------------------------------------------------------------
// Core engine modules
// ------------------------------------------------------------
import aengine.core.context;
import aengine.context.commandqueue;
import aengine.context.window;
import aengine.input;
import aplatformpump;
import aatlas.manager;
import aengine.core.commandline;

// ------------------------------------------------------------
// OpenGL backend modules
// ------------------------------------------------------------
import acontext.opengl.textures;
import acontext.opengl.state;
import acontext.opengl.platform;

// ------------------------------------------------------------
// Standard library
// ------------------------------------------------------------
import <algorithm>;
import <cstdint>;
import <format>;
import <functional>;
import <iostream>;
import <stdexcept>;
import <string>;
import <string_view>;
import <utility>;
import <vector>;

export namespace almondnamespace::openglcontext
{
#if !defined(ALMOND_USING_OPENGL)

    inline bool opengl_initialize(std::shared_ptr<core::Context>, void* = nullptr,
        unsigned int = 0, unsigned int = 0, std::function<void(int, int)> = nullptr) {
        return false;
    }

    inline void opengl_present() {}
    inline int  opengl_get_width() { return 1; }
    inline int  opengl_get_height() { return 1; }
    inline void opengl_clear() {}
    inline bool opengl_process(std::shared_ptr<core::Context>, core::CommandQueue&) { return false; }
    inline void opengl_cleanup(std::shared_ptr<core::Context>) {}

#else

    namespace detail
    {
#if defined(__linux__)
        inline ::Window to_xwindow(void* opaque) noexcept
        {
            return static_cast<::Window>(reinterpret_cast<std::uintptr_t>(opaque));
        }
        inline void* from_xwindow(::Window window) noexcept
        {
            return reinterpret_cast<void*>(static_cast<std::uintptr_t>(window));
        }
#endif

        inline PlatformGL::PlatformGLContext state_to_platform_context(const almondnamespace::openglstate::OpenGL4State& state) noexcept
        {
            PlatformGL::PlatformGLContext ctx{};
#if defined(_WIN32)
            ctx.device = state.hdc;
            ctx.context = state.hglrc;
#elif defined(__linux__)
            ctx.display = state.display;
            ctx.drawable = state.drawable ? state.drawable : state.window;
            ctx.context = state.glxContext;
#endif
            return ctx;
        }

        inline PlatformGL::PlatformGLContext context_to_platform_context(const core::Context* ctx) noexcept
        {
            PlatformGL::PlatformGLContext result{};
            if (!ctx) return result;

#if defined(_WIN32)
            if (ctx->windowData)
            {
                result.device = ctx->windowData->hdc;
                result.context = ctx->windowData->glContext;
                return result;
            }
            result.device = static_cast<HDC>(ctx->native_drawable);
            result.context = static_cast<HGLRC>(ctx->native_gl_context);
#elif defined(__linux__)
            result.display = static_cast<Display*>(ctx->native_drawable);
            result.drawable = static_cast<GLXDrawable>(reinterpret_cast<std::uintptr_t>(ctx->native_window));
            result.context = static_cast<GLXContext>(ctx->native_gl_context);
#endif
            return result;
        }

#if defined(_WIN32)
        inline bool is_bad_wgl_ptr(void* p) noexcept
        {
            const auto v = reinterpret_cast<std::uintptr_t>(p);
            return v == 0u || v == 1u || v == 2u || v == 3u || v == static_cast<std::uintptr_t>(-1);
        }
#endif
    } // namespace detail

    inline void* LoadGLFunc(const char* name)
    {
        return PlatformGL::get_proc_address(name);
    }

    inline GLuint compileShader(GLenum type, const char* src)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char log[512]{};
            glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(log)), nullptr, log);
            throw std::runtime_error(std::format("Shader compile error: {}\nSource:\n{}", log, src));
        }
        return shader;
    }

    inline GLuint linkProgram(const char* vsSrc, const char* fsSrc)
    {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSrc);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char log[512]{};
            glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
            throw std::runtime_error(std::format("Program link error: {}", log));
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return program;
    }

    inline const char* select_glsl_version(GLint major, GLint minor)
    {
        struct V { GLint major; GLint minor; const char* sv; };
        static constexpr V versions[] = {
            {4, 6, "#version 460 core"},
            {4, 5, "#version 450 core"},
            {4, 4, "#version 440 core"},
            {4, 3, "#version 430 core"},
            {4, 2, "#version 420 core"},
            {4, 1, "#version 410 core"},
            {4, 0, "#version 400 core"},
            {3, 3, "#version 330 core"},
        };

        for (const auto& c : versions)
            if (major > c.major || (major == c.major && minor >= c.minor))
                return c.sv;

        return "#version 330 core";
    }

    inline std::pair<GLint, GLint> parse_gl_version_string(const char* versionStr) noexcept
    {
        if (!versionStr) return { 0, 0 };

        std::string_view sv{ versionStr };
        auto p = sv.find_first_of("0123456789");
        if (p == std::string_view::npos) return { 0, 0 };
        sv.remove_prefix(p);

        auto dot = sv.find('.');
        if (dot == std::string_view::npos) return { 0, 0 };

        auto major_part = sv.substr(0, dot);
        sv.remove_prefix(dot + 1);

        auto minor_end = sv.find_first_not_of("0123456789");
        auto minor_part = sv.substr(0, minor_end);

        auto parse = [](std::string_view s) noexcept -> GLint {
            GLint v = 0;
            for (unsigned char ch : s)
            {
                if (ch < '0' || ch > '9') return 0;
                v = static_cast<GLint>(v * 10 + (ch - '0'));
            }
            return v;
            };

        return { parse(major_part), parse(minor_part) };
    }

    inline void destroy_quad_pipeline(almondnamespace::openglstate::OpenGL4State& glState) noexcept
    {
        if (glState.shader && glIsProgram(glState.shader))
            glDeleteProgram(glState.shader);

        glState.shader = 0;
        glState.uUVRegionLoc = -1;
        glState.uTransformLoc = -1;
        glState.uSamplerLoc = -1;

        if (glState.vao && glIsVertexArray(glState.vao))
            glDeleteVertexArrays(1, &glState.vao);
        if (glState.vbo && glIsBuffer(glState.vbo))
            glDeleteBuffers(1, &glState.vbo);
        if (glState.ebo && glIsBuffer(glState.ebo))
            glDeleteBuffers(1, &glState.ebo);

        glState.vao = 0;
        glState.vbo = 0;
        glState.ebo = 0;
    }

    inline bool build_quad_pipeline(almondnamespace::openglstate::OpenGL4State& glState)
    {
        destroy_quad_pipeline(glState);

        GLint major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        if (major == 0 && minor == 0)
        {
            const auto* vs = reinterpret_cast<const char*>(glGetString(GL_VERSION));
            auto [pm, pn] = parse_gl_version_string(vs);
            major = pm; minor = pn;
        }
        if (major == 0 && minor == 0) { major = 3; minor = 3; }

        const char* directive = select_glsl_version(major, minor);

        std::string vs_source = std::string(directive) + R"(

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform vec4 uTransform; // xy=center in NDC, zw=size in NDC
uniform vec4 uUVRegion;  // xy=uv offset, zw=uv size

out vec2 vUV;

void main() {
    vec2 pos = aPos * uTransform.zw + uTransform.xy;
    gl_Position = vec4(pos, 0.0, 1.0);
    vUV = uUVRegion.xy + aTexCoord * uUVRegion.zw;
}
)";

        std::string fs_source = std::string(directive) + R"(

in vec2 vUV;
out vec4 outColor;

uniform sampler2D uTexture;

void main() {
    outColor = texture(uTexture, vUV);
}
)";

        try
        {
            glState.shader = linkProgram(vs_source.c_str(), fs_source.c_str());
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[OpenGL] Pipeline build failed: " << ex.what() << "\n";
            destroy_quad_pipeline(glState);
            return false;
        }

        glState.uUVRegionLoc = glGetUniformLocation(glState.shader, "uUVRegion");
        glState.uTransformLoc = glGetUniformLocation(glState.shader, "uTransform");
        glState.uSamplerLoc = glGetUniformLocation(glState.shader, "uTexture");

        if (glState.uSamplerLoc >= 0)
        {
            glUseProgram(glState.shader);
            glUniform1i(glState.uSamplerLoc, 0);
            glUseProgram(0);
        }

        glGenVertexArrays(1, &glState.vao);
        glGenBuffers(1, &glState.vbo);
        glGenBuffers(1, &glState.ebo);

        if (!glState.vao || !glState.vbo || !glState.ebo)
        {
            std::cerr << "[OpenGL] Failed to allocate quad buffers\n";
            destroy_quad_pipeline(glState);
            return false;
        }

        glBindVertexArray(glState.vao);
        glBindBuffer(GL_ARRAY_BUFFER, glState.vbo);

        constexpr float quadVerts[] = {
            -0.5f, -0.5f, 0.0f, 0.0f,
             0.5f, -0.5f, 1.0f, 0.0f,
             0.5f,  0.5f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 1.0f
        };
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(quadVerts)), quadVerts, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * static_cast<GLsizei>(sizeof(float)), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * static_cast<GLsizei>(sizeof(float)),
            (void*)(2 * sizeof(float)));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glState.ebo);
        constexpr unsigned int quadIdx[] = { 0, 1, 2, 2, 3, 0 };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(quadIdx)), quadIdx, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        return true;
    }

    inline bool ensure_quad_pipeline(almondnamespace::openglstate::OpenGL4State& glState)
    {
        const bool shaderValid = glState.shader != 0 && glIsProgram(glState.shader) == GL_TRUE;
        const bool vaoValid = glState.vao != 0 && glIsVertexArray(glState.vao) == GL_TRUE;
        const bool vboValid = glState.vbo != 0 && glIsBuffer(glState.vbo) == GL_TRUE;
        const bool eboValid = glState.ebo != 0 && glIsBuffer(glState.ebo) == GL_TRUE;

        if (shaderValid && vaoValid && vboValid && eboValid)
            return true;

        return build_quad_pipeline(glState);
    }

    // ------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------
    inline bool opengl_initialize(std::shared_ptr<core::Context> ctx,
        void* parentWindowOpaque = nullptr,
        unsigned int w = 800,
        unsigned int h = 600,
        std::function<void(int, int)> onResize = nullptr)
    {
        if (!ctx)
            throw std::runtime_error("[OpenGL] opengl_initialize requires non-null Context");

        auto& backend = almondnamespace::opengltextures::get_opengl_backend();
        auto& glState = backend.glState;

        glState.width = w;
        glState.height = h;
        ctx->onResize = std::move(onResize);

        PlatformGL::ScopedContext contextGuard;

#if defined(_WIN32)
        HWND parentHwnd = static_cast<HWND>(parentWindowOpaque);
        if (ctx->windowData && ctx->windowData->hwnd)
            parentHwnd = ctx->windowData->hwnd;

        if (!parentHwnd)
            throw std::runtime_error("[OpenGL] No parent HWND available");

        // Avoid custom window class registration: use a stock class.
        if (!glState.hwnd)
        {
            glState.parent = parentHwnd;
            glState.hwnd = ::CreateWindowExW(
                0,
                L"STATIC",
                L"",
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                0, 0, static_cast<int>(w), static_cast<int>(h),
                glState.parent,
                nullptr,
                ::GetModuleHandleW(nullptr),
                nullptr);

            if (!glState.hwnd)
                throw std::runtime_error("[OpenGL] CreateWindowExW failed for child GL window");
        }

        glState.hdc = ::GetDC(glState.hwnd);
        if (!glState.hdc)
            throw std::runtime_error("[OpenGL] GetDC failed");

        // SetPixelFormat is one-time per HDC.
        if (::GetPixelFormat(glState.hdc) == 0)
        {
            PIXELFORMATDESCRIPTOR pfd{
                sizeof(PIXELFORMATDESCRIPTOR), 1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                PFD_TYPE_RGBA, 32,
                0,0,0,0,0,0,0,0,0,
                24, 0, 0, 0, 0, 0, 0, 0
            };

            int pf = ::ChoosePixelFormat(glState.hdc, &pfd);
            if (!pf) throw std::runtime_error("[OpenGL] ChoosePixelFormat failed");
            if (!::SetPixelFormat(glState.hdc, pf, &pfd))
                throw std::runtime_error("[OpenGL] SetPixelFormat failed");
        }

        // ---- WGL bootstrap (correctly): temp context stays current while loading + creating ----
        HGLRC tmp = ::wglCreateContext(glState.hdc);
        if (!tmp) throw std::runtime_error("[OpenGL] wglCreateContext(temp) failed");
        if (::wglMakeCurrent(glState.hdc, tmp) != TRUE)
        {
            ::wglDeleteContext(tmp);
            throw std::runtime_error("[OpenGL] wglMakeCurrent(temp) failed");
        }

        // Load wglCreateContextAttribsARB safely (filter WGL sentinel pointers).
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
        {
            void* raw = reinterpret_cast<void*>(::wglGetProcAddress("wglCreateContextAttribsARB"));
            if (!detail::is_bad_wgl_ptr(raw))
                wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(raw);
        }

        // Try modern core context; if it fails or extension missing, keep the temp legacy context as final.
        glState.hglrc = nullptr;

        if (wglCreateContextAttribsARB)
        {
            int attribs46[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                WGL_CONTEXT_MINOR_VERSION_ARB, 6,
                WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };

            glState.hglrc = wglCreateContextAttribsARB(glState.hdc, nullptr, attribs46);

            if (!glState.hglrc)
            {
                int attribs41[] = {
                    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                    WGL_CONTEXT_MINOR_VERSION_ARB, 1,
                    WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                    0
                };
                glState.hglrc = wglCreateContextAttribsARB(glState.hdc, nullptr, attribs41);
            }
        }

        if (glState.hglrc)
        {
            // Switch to final and destroy temp.
            (void)::wglMakeCurrent(nullptr, nullptr);
            ::wglDeleteContext(tmp);
            tmp = nullptr;

            if (::wglMakeCurrent(glState.hdc, glState.hglrc) != TRUE)
            {
                ::wglDeleteContext(glState.hglrc);
                glState.hglrc = nullptr;
                throw std::runtime_error("[OpenGL] wglMakeCurrent(final) failed");
            }
        }
        else
        {
            // No modern context: the temp legacy context becomes the final context.
            glState.hglrc = tmp;
            tmp = nullptr;
        }

        // Publish through PlatformGL (so the rest of the engine uses the same path).
        {
            PlatformGL::PlatformGLContext finalCtx{};
            finalCtx.device = glState.hdc;
            finalCtx.context = glState.hglrc;
            if (!contextGuard.set(finalCtx))
                throw std::runtime_error("[OpenGL] PlatformGL::make_current(final) failed");
        }

        // Load GL entry points.
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(PlatformGL::get_proc_address)))
            throw std::runtime_error("[OpenGL] gladLoadGLLoader failed");

        ctx->native_window = glState.hwnd;
        ctx->native_drawable = glState.hdc;
        ctx->native_gl_context = glState.hglrc;

#elif defined(__linux__)
        (void)parentWindowOpaque;

        Display* display = glState.display ? glState.display : XOpenDisplay(nullptr);
        if (!display)
            throw std::runtime_error("[OpenGL] XOpenDisplay failed");
        glState.display = display;

        const int screen = DefaultScreen(display);

        int fbCount = 0;
        int visualAttribs[] = {
            GLX_X_RENDERABLE, True,
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
            GLX_DOUBLEBUFFER, True,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8,
            GLX_DEPTH_SIZE, 24,
            0
        };

        GLXFBConfig* configs = glXChooseFBConfig(display, screen, visualAttribs, &fbCount);
        if (!configs || fbCount == 0)
            throw std::runtime_error("[OpenGL] glXChooseFBConfig failed");

        glState.fbConfig = configs[0];
        XFree(configs);

        XVisualInfo* vi = glXGetVisualFromFBConfig(display, glState.fbConfig);
        if (!vi)
            throw std::runtime_error("[OpenGL] glXGetVisualFromFBConfig failed");

        if (!glState.colormap)
        {
            glState.colormap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
            if (!glState.colormap)
            {
                XFree(vi);
                throw std::runtime_error("[OpenGL] XCreateColormap failed");
            }
        }

        if (!glState.window)
        {
            XSetWindowAttributes swa{};
            swa.colormap = glState.colormap;
            swa.event_mask = ExposureMask | StructureNotifyMask;

            glState.window = XCreateWindow(
                display, RootWindow(display, vi->screen),
                0, 0, w, h, 0, vi->depth, InputOutput, vi->visual,
                CWColormap | CWEventMask, &swa);

            if (!glState.window)
            {
                XFree(vi);
                throw std::runtime_error("[OpenGL] XCreateWindow failed");
            }

            XStoreName(display, glState.window, "Almond OpenGL");
            XMapWindow(display, glState.window);
            XFlush(display);
        }

        XFree(vi);
        glState.drawable = glState.window;

        if (!glState.glxContext)
        {
            auto createContextAttribs =
                reinterpret_cast<PFNGLXCREATECONTEXTATTRIBSARBPROC>(LoadGLFunc("glXCreateContextAttribsARB"));

            int contextAttribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 3,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };

            if (createContextAttribs)
                glState.glxContext = createContextAttribs(display, glState.fbConfig, nullptr, True, contextAttribs);

            if (!glState.glxContext)
                glState.glxContext = glXCreateNewContext(display, glState.fbConfig, GLX_RGBA_TYPE, nullptr, True);

            if (!glState.glxContext)
                throw std::runtime_error("[OpenGL] Failed to create GLX context");
        }

        {
            PlatformGL::PlatformGLContext finalCtx{};
            finalCtx.display = display;
            finalCtx.drawable = glState.drawable;
            finalCtx.context = glState.glxContext;
            if (!contextGuard.set(finalCtx))
                throw std::runtime_error("[OpenGL] glXMakeCurrent failed");
        }

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(PlatformGL::get_proc_address)))
            throw std::runtime_error("[OpenGL] gladLoadGLLoader failed");

        ctx->native_window = reinterpret_cast<void*>(static_cast<std::uintptr_t>(glState.window));
        ctx->native_drawable = display;
        ctx->native_gl_context = glState.glxContext;
#else
        (void)parentWindowOpaque;
        throw std::runtime_error("[OpenGL] Unsupported platform");
#endif

        if (!ensure_quad_pipeline(glState))
            throw std::runtime_error("[OpenGL] Failed to build/ensure quad pipeline");

        atlasmanager::register_backend_uploader(core::ContextType::OpenGL,
            [](const TextureAtlas& atlas) { opengltextures::ensure_uploaded(atlas); });

        contextGuard.release();
        return true;
    }

    inline void opengl_present()
    {
        auto& backend = opengltextures::get_opengl_backend();
        auto ctx = detail::state_to_platform_context(backend.glState);
        PlatformGL::swap_buffers(ctx);
    }

    inline int opengl_get_width()
    {
        auto& backend = opengltextures::get_opengl_backend();
        if (backend.glState.width > 0)
            return static_cast<int>(backend.glState.width);
        return (std::max)(1, core::cli::window_width);
    }

    inline int opengl_get_height()
    {
        auto& backend = opengltextures::get_opengl_backend();
        if (backend.glState.height > 0)
            return static_cast<int>(backend.glState.height);
        return (std::max)(1, core::cli::window_height);
    }

    inline void opengl_clear()
    {
        glClearColor(0.235f, 0.235f, 0.235f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    inline bool opengl_process(std::shared_ptr<core::Context> ctx, core::CommandQueue& queue)
    {
        if (!ctx) return false;

        auto& backend = opengltextures::get_opengl_backend();
        auto& glState = backend.glState;

        PlatformGL::ScopedContext guard;
        auto desired = detail::context_to_platform_context(ctx.get());
        if (!desired.valid() || !guard.set(desired))
        {
            auto fallback = detail::state_to_platform_context(glState);
            if (!fallback.valid() || !guard.set(fallback))
                return false;
        }

        atlasmanager::process_pending_uploads(core::ContextType::OpenGL);

        if (!ensure_quad_pipeline(glState))
        {
            queue.drain();
            PlatformGL::swap_buffers(guard.target());
            return true;
        }

        ctx->is_key_held = [](almondnamespace::input::Key k) { return almondnamespace::input::is_key_held(k); };
        ctx->is_key_down = [](almondnamespace::input::Key k) { return almondnamespace::input::is_key_down(k); };
        ctx->is_mouse_button_held = [](almondnamespace::input::MouseButton b) { return almondnamespace::input::is_mouse_button_held(b); };
        ctx->is_mouse_button_down = [](almondnamespace::input::MouseButton b) { return almondnamespace::input::is_mouse_button_down(b); };

        const int fbW = (std::max)(1, opengl_get_width());
        const int fbH = (std::max)(1, opengl_get_height());
        glViewport(0, 0, fbW, fbH);

        opengl_clear();
        queue.drain();

        PlatformGL::swap_buffers(guard.target());
        return true;
    }

    inline void opengl_cleanup(std::shared_ptr<core::Context> ctx)
    {
        auto& backend = opengltextures::get_opengl_backend();
        auto& glState = backend.glState;
        (void)ctx;

#if defined(_WIN32)
        PlatformGL::clear_current();

        if (glState.hglrc) { ::wglDeleteContext(glState.hglrc); glState.hglrc = nullptr; }
        if (glState.hdc && glState.hwnd) { ::ReleaseDC(glState.hwnd, glState.hdc); }
        glState.hdc = nullptr;

        if (glState.hwnd) { ::DestroyWindow(glState.hwnd); glState.hwnd = nullptr; }
        glState.parent = nullptr;

#elif defined(__linux__)
        PlatformGL::clear_current();
        if (glState.display && glState.glxContext) glXDestroyContext(glState.display, glState.glxContext);
        if (glState.display && glState.window) XDestroyWindow(glState.display, glState.window);
        if (glState.display && glState.colormap) XFreeColormap(glState.display, glState.colormap);
        if (glState.display) XCloseDisplay(glState.display);

        glState.display = nullptr;
        glState.window = 0;
        glState.drawable = 0;
        glState.glxContext = nullptr;
        glState.colormap = 0;
        glState.fbConfig = nullptr;
#endif
    }

#endif // ALMOND_USING_OPENGL
} // namespace almondnamespace::openglcontext










