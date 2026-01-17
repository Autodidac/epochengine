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
 // aengine.context.multiplexer.win.cppm
module;

// configuration
#include "..\\include\\aengine.config.hpp"

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <windowsx.h>
#   include <commctrl.h>
#   include <shellapi.h>
#   pragma comment(lib, "comctl32.lib")

#   include <algorithm>
#   include <chrono>
#   include <functional>
#   include <iostream>
#   include <memory>
#   include <shared_mutex>
#   include <stdexcept>
#   include <string>
#   include <string_view>
#   include <thread>
#   include <unordered_map>
#   include <utility>
#   include <vector>

#   include <glad/glad.h>
#endif

export module aengine.context.multiplexer:win;

import aengine.platform;
import autility.string.converter;

import aengine.cli;
import aengine.core.context;

import aengine.context.commandqueue;
import aengine.context.multiplexer;
import aengine.context.type;
import aengine.context.window;

#if defined(ALMOND_USING_OPENGL)
import acontext.opengl.context;
#endif
#if defined(ALMOND_USING_SDL)
import acontext.sdl.context;
#endif
#if defined(ALMOND_USING_RAYLIB)
import acontext.raylib.context;
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
import acontext.softrenderer.context;
#endif

#if defined(_WIN32)

namespace
{
    // TU-owned globals. Accessed via almondnamespace::core::Threads()/Drag().
    std::unordered_map<HWND, std::thread> g_threads;
    almondnamespace::core::DragState       g_drag;

#if ALMOND_SINGLE_PARENT
    struct SubCtx { HWND originalParent{}; };

    LRESULT CALLBACK DockableProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR dw)
    {
        auto* ctx = reinterpret_cast<SubCtx*>(dw);

        switch (msg)
        {
        case WM_NCDESTROY:
            // We allocated SubCtx in MakeDockable; free it here.
            delete ctx;
            return DefSubclassProc(hwnd, msg, wp, lp);

        case WM_CLOSE:
            // Child close should not kill the process when docked.
            // Only the actual *parent* window closing triggers WM_QUIT (handled in ParentProc).
            ::DestroyWindow(hwnd);
            return 0;

        case WM_LBUTTONDOWN:
        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
            return almondnamespace::core::MultiContextManager::ChildProc(hwnd, msg, wp, lp);
        }

        return DefSubclassProc(hwnd, msg, wp, lp);
    }
#endif

    [[nodiscard]] inline int clamp_positive(int v) noexcept { return (v < 1) ? 1 : v; }
}

namespace almondnamespace::core
{
    // ------------------------------------------------------------
    // Exported helpers
    // ------------------------------------------------------------
    std::unordered_map<HWND, std::thread>& Threads() noexcept { return g_threads; }
    DragState& Drag() noexcept { return g_drag; }

    void MakeDockable(HWND hwnd, HWND parent)
    {
#if ALMOND_SINGLE_PARENT
        (void)parent;
        if (!hwnd) return;
        auto* ctx = new SubCtx{ parent };
        if (!::SetWindowSubclass(hwnd, DockableProc, 1, reinterpret_cast<DWORD_PTR>(ctx)))
        {
            delete ctx;
        }
#else
        (void)hwnd;
        (void)parent;
#endif
    }

    namespace backend
    {
        std::wstring_view BackendDisplayName(ContextType type) noexcept
        {
            switch (type)
            {
            case ContextType::OpenGL:   return L"OpenGL";
            case ContextType::SDL:      return L"SDL";
            case ContextType::SFML:     return L"SFML";
            case ContextType::RayLib:   return L"Raylib";
            case ContextType::Software: return L"Software";
            case ContextType::Vulkan:   return L"Vulkan";
            case ContextType::DirectX:  return L"DirectX";
            case ContextType::Noop:     return L"Noop";
            case ContextType::Custom:   return L"Custom";
            default:                    return L"Context";
            }
        }

        std::wstring BuildChildWindowTitle(ContextType type, int index)
        {
            const std::wstring_view base = BackendDisplayName(type);
            std::wstring title{ base.begin(), base.end() };
            title += L" Dock ";
            title += std::to_wstring(static_cast<long long>(index) + 1);
            return title;
        }

        void ResolveClientSize(HWND hwnd, int& width, int& height) noexcept
        {
            if (!hwnd) return;
            RECT client{};
            if (!::GetClientRect(hwnd, &client)) return;
            width = clamp_positive(static_cast<int>(client.right - client.left));
            height = clamp_positive(static_cast<int>(client.bottom - client.top));
        }
    }

    // ------------------------------------------------------------
    // Window lookup
    // ------------------------------------------------------------
    WindowData* MultiContextManager::findWindowByHWND(HWND hwnd)
    {
        std::scoped_lock lock(windowsMutex);
        auto it = std::find_if(windows.begin(), windows.end(),
            [hwnd](const std::unique_ptr<WindowData>& w) { return w && w->hwnd == hwnd; });
        return (it != windows.end()) ? it->get() : nullptr;
    }

    const WindowData* MultiContextManager::findWindowByHWND(HWND hwnd) const
    {
        std::scoped_lock lock(windowsMutex);
        auto it = std::find_if(windows.begin(), windows.end(),
            [hwnd](const std::unique_ptr<WindowData>& w) { return w && w->hwnd == hwnd; });
        return (it != windows.end()) ? it->get() : nullptr;
    }

    WindowData* MultiContextManager::findWindowByContext(const std::shared_ptr<Context>& ctx)
    {
        if (!ctx) return nullptr;
        std::scoped_lock lock(windowsMutex);
        auto it = std::find_if(windows.begin(), windows.end(),
            [&](const std::unique_ptr<WindowData>& w) { return w && w->context && w->context.get() == ctx.get(); });
        return (it != windows.end()) ? it->get() : nullptr;
    }

    const WindowData* MultiContextManager::findWindowByContext(const std::shared_ptr<Context>& ctx) const
    {
        if (!ctx) return nullptr;
        std::scoped_lock lock(windowsMutex);
        auto it = std::find_if(windows.begin(), windows.end(),
            [&](const std::unique_ptr<WindowData>& w) { return w && w->context && w->context.get() == ctx.get(); });
        return (it != windows.end()) ? it->get() : nullptr;
    }

    // ------------------------------------------------------------
    // MultiContextManager (static)
    // ------------------------------------------------------------
    //void MultiContextManager::SetCurrent(std::shared_ptr<Context> ctx)
    //{
    //    // Must qualify: currentContext is a static data member of the class.
    //    MultiContextManager::currentContext = std::move(ctx);
    //}

    //std::shared_ptr<Context> MultiContextManager::GetCurrent()
    //{
    //    return MultiContextManager::currentContext;
    //}



    // ------------------------------------------------------------
    // MultiContextManager (public helpers)
    // ------------------------------------------------------------
    bool MultiContextManager::IsRunning() const noexcept
    {
        return running.load(std::memory_order_acquire);
    }

    void MultiContextManager::StopRunning() noexcept
    {
        running.store(false, std::memory_order_release);
    }

    void MultiContextManager::EnqueueRenderCommand(HWND hwnd, RenderCommand cmd)
    {
        std::scoped_lock lock(windowsMutex);
        auto it = std::find_if(windows.begin(), windows.end(),
            [hwnd](const std::unique_ptr<WindowData>& w) { return w && w->hwnd == hwnd; });
        if (it != windows.end()) (*it)->EnqueueCommand(std::move(cmd));
    }

    // ------------------------------------------------------------
    // Implementation
    // ------------------------------------------------------------
    void MultiContextManager::ShowConsole()
    {
        if (::AllocConsole())
        {
            FILE* f{};
            freopen_s(&f, "CONOUT$", "w", stdout);
            freopen_s(&f, "CONIN$", "r", stdin);
            freopen_s(&f, "CONOUT$", "w", stderr);
            std::ios::sync_with_stdio(true);
        }
    }

    ATOM MultiContextManager::RegisterParentClass(HINSTANCE hInst, LPCWSTR name)
    {
        WNDCLASSW wc{};
        wc.lpfnWndProc = ParentProc;
        wc.hInstance = hInst;
        wc.lpszClassName = name;
        wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
        return ::RegisterClassW(&wc);
    }

    ATOM MultiContextManager::RegisterChildClass(HINSTANCE hInst, LPCWSTR name)
    {
        WNDCLASSW wc{};
        wc.lpfnWndProc = ChildProc;
        wc.hInstance = hInst;
        wc.lpszClassName = name;
        wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        return ::RegisterClassW(&wc);
    }

    void MultiContextManager::SetupPixelFormat(HDC hdc)
    {
        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.iLayerType = PFD_MAIN_PLANE;

        const int pf = ::ChoosePixelFormat(hdc, &pfd);
        if (pf == 0 || !::SetPixelFormat(hdc, pf, &pfd))
            throw std::runtime_error("Failed to set pixel format");
    }

    HGLRC MultiContextManager::CreateSharedGLContext(HDC hdc)
    {
        SetupPixelFormat(hdc);
        HGLRC ctx = ::wglCreateContext(hdc);
        if (!ctx) throw std::runtime_error("Failed to create OpenGL context");

        if (sharedContext && !::wglShareLists(sharedContext, ctx))
        {
            ::wglDeleteContext(ctx);
            throw std::runtime_error("Failed to share GL context");
        }

        return ctx;
    }

    int MultiContextManager::get_title_bar_thickness(const HWND window_handle)
    {
        RECT wr{}, cr{};
        ::GetWindowRect(window_handle, &wr);
        ::GetClientRect(window_handle, &cr);

        const int totalH = wr.bottom - wr.top;
        const int clientH = cr.bottom - cr.top;
        const int totalW = wr.right - wr.left;
        const int clientW = cr.right - cr.left;

        const int nonClientH = totalH - clientH;
        const int nonClientW = totalW - clientW;

        const int border = nonClientW / 2;
        const int title = nonClientH - border * 2;
        return title;
    }

    bool MultiContextManager::Initialize(
        HINSTANCE hInst,
        int RayLibWinCount,
        int SDLWinCount,
        int SFMLWinCount,
        int OpenGLWinCount,
        int SoftwareWinCount,
        bool parented)
    {
        const int totalRequested = RayLibWinCount + SDLWinCount + SFMLWinCount + OpenGLWinCount + SoftwareWinCount;
        if (totalRequested <= 0) return false;

        running.store(true, std::memory_order_release);
        s_activeInstance = this;

        RegisterParentClass(hInst, L"AlmondParent");
        RegisterChildClass(hInst, L"AlmondChild");

        almondnamespace::core::InitializeAllContexts();

        // ---------------- Parent (dock container) ----------------
        if (parented)
        {
            int cols = 1, rows = 1;
            while (cols * rows < totalRequested) (cols <= rows ? ++cols : ++rows);

            const int cellW = (totalRequested == 1) ? cli::window_width : 800;
            const int cellH = (totalRequested == 1) ? cli::window_height : 600;

            const int clientW = cols * cellW;
            const int clientH = rows * cellH;

            RECT want{ 0, 0, clientW, clientH };
            const DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
            ::AdjustWindowRect(&want, style, FALSE);

            parent = ::CreateWindowExW(
                0,
                L"AlmondParent",
                L"Almond Docking",
                style,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                want.right - want.left,
                want.bottom - want.top,
                nullptr,
                nullptr,
                hInst,
                this);

            if (!parent) return false;
            ::DragAcceptFiles(parent, TRUE);
        }
        else
        {
            parent = nullptr;
        }

#if defined(ALMOND_USING_OPENGL)
        // ---------------- Shared dummy GL context (for wglShareLists + glad bootstrap) ----------------
        {
            HWND dummy = ::CreateWindowExW(
                WS_EX_TOOLWINDOW,
                L"AlmondChild",
                L"Dummy",
                WS_POPUP,
                0, 0, 1, 1,
                nullptr,
                nullptr,
                hInst,
                nullptr);

            if (!dummy) return false;

            HDC dummyDC = ::GetDC(dummy);
            SetupPixelFormat(dummyDC);
            sharedContext = ::wglCreateContext(dummyDC);
            if (!sharedContext)
            {
                ::ReleaseDC(dummy, dummyDC);
                ::DestroyWindow(dummy);
                throw std::runtime_error("Failed to create shared OpenGL context");
            }

            ::wglMakeCurrent(dummyDC, sharedContext);

            static bool gladInitialized = false;
            if (!gladInitialized)
            {
                gladInitialized = (gladLoadGL() != 0);
                std::cerr << "[Init] GLAD loaded on dummy context\n";
            }

            ::wglMakeCurrent(nullptr, nullptr);
            ::ReleaseDC(dummy, dummyDC);
            ::DestroyWindow(dummy);
        }
#endif

        // ---------------- Helper: create N windows for a backend ----------------
        auto make_backend_windows = [&](ContextType type, int count)
            {
                if (count <= 0) return;

                std::vector<HWND> created;
                created.reserve(static_cast<size_t>(count));
                std::vector<std::string> createdTitles;
                createdTitles.reserve(static_cast<size_t>(count));

                for (int i = 0; i < count; ++i)
                {
                    const std::wstring windowTitle = backend::BuildChildWindowTitle(type, i);
                    const std::string narrowTitle = almondnamespace::text::narrow_utf8(windowTitle);

                    HWND hwnd = ::CreateWindowExW(
                        0,
                        L"AlmondChild",
                        windowTitle.c_str(),
                        (parent ? (WS_CHILD | WS_VISIBLE) : (WS_OVERLAPPEDWINDOW | WS_VISIBLE)),
                        0, 0, 800, 600,
                        parent,
                        nullptr,
                        hInst,
                        nullptr);

                    if (!hwnd) continue;

                    ::SetWindowTextW(hwnd, windowTitle.c_str());

                    HDC hdc = ::GetDC(hwnd);
                    HGLRC glrc = nullptr;

#if defined(ALMOND_USING_OPENGL)
                    if (type == ContextType::OpenGL)
                    {
                        glrc = CreateSharedGLContext(hdc);
                    }
#endif

                    auto winPtr = std::make_unique<contextwindow::WindowData>(hwnd, hdc, glrc, true, type);
                    winPtr->running = true; // critical: default-constructed WindowData is not guaranteed to be running
                    winPtr->titleWide = windowTitle;
                    winPtr->titleNarrow = narrowTitle;

                    if (parent) MakeDockable(hwnd, parent);

                    {
                        std::scoped_lock lock(windowsMutex);
                        windows.emplace_back(std::move(winPtr));
                    }

                    created.push_back(hwnd);
                    createdTitles.push_back(narrowTitle);
                }

                std::vector<std::shared_ptr<Context>> ctxs;

                // IMPORTANT: do not call CloneContext() while holding g_backendsMutex.
                // If CloneContext() ever evolves to touch g_backends/g_backendsMutex (directly or indirectly),
                // calling it under this lock will deadlock.
                std::shared_ptr<Context> master;
                std::vector<std::shared_ptr<Context>> free_dups;
                free_dups.reserve(static_cast<size_t>((std::max)(0, count - 1)));

                {
                    std::unique_lock lock(g_backendsMutex);
                    auto it = g_backends.find(type);
                    if (it == g_backends.end() || !it->second.master)
                    {
                        std::cerr << "[Init] Missing prototype context for backend type " << static_cast<int>(type) << "\n";
                        return;
                    }

                    BackendState& state = it->second;
                    master = state.master;

                    for (auto& dup : state.duplicates)
                    {
                        if (dup && dup->windowData == nullptr)
                            free_dups.push_back(dup);
                    }
                }

                if (!master)
                    return;

                ctxs.reserve(static_cast<size_t>(count));
                ctxs.push_back(master);

                while (ctxs.size() < static_cast<size_t>(count))
                {
                    if (!free_dups.empty())
                    {
                        ctxs.push_back(std::move(free_dups.back()));
                        free_dups.pop_back();
                        continue;
                    }

                    auto cloned = CloneContext(*master);
                    {
                        std::unique_lock lock(g_backendsMutex);
                        auto& state = g_backends[type];
                        state.duplicates.push_back(cloned);
                    }
                    ctxs.push_back(std::move(cloned));
                }

                const size_t n = (std::min)(created.size(), ctxs.size());
                for (size_t i = 0; i < n; ++i)
                {
                    HWND hwnd = created[i];
                    auto* w = findWindowByHWND(hwnd);
                    auto& ctx = ctxs[i];
                    if (!ctx) continue;

                    ctx->type = type;
                    ctx->hwnd = hwnd;

                    int width = 800;
                    int height = 600;
                    backend::ResolveClientSize(hwnd, width, height);
                    ctx->width = width;
                    ctx->height = height;

                    std::string narrowTitle;
                    if (w)
                    {
                        ctx->hdc = w->hdc;
                        ctx->hglrc = w->glContext;
                        ctx->windowData = w;
                        w->context = ctx;
                        w->width = width;
                        w->height = height;

                        // Ensure WindowData starts "alive".
                        w->running = true;

                        if (!ctx->onResize && w->onResize) ctx->onResize = w->onResize;
                        if (!w->titleNarrow.empty()) narrowTitle = w->titleNarrow;
                    }

                    if (narrowTitle.empty())
                    {
                        narrowTitle = (i < createdTitles.size()) ? createdTitles[i]
                            : almondnamespace::text::narrow_utf8(backend::BuildChildWindowTitle(type, static_cast<int>(i)));
                    }

                    const HWND placeholderHwnd = hwnd;
                    const HWND hostParent = parent;

                    auto adopt_backend_window = [&](HWND actualHwnd)
                        {
                            if (!w || !ctx || !actualHwnd || actualHwnd == placeholderHwnd) return;

                            const HWND previous = w->hwnd;

                            if (hostParent)
                            {
                                ::SetParent(actualHwnd, hostParent);

                                LONG_PTR style = ::GetWindowLongPtr(actualHwnd, GWL_STYLE);
                                style &= ~(WS_POPUP | WS_OVERLAPPEDWINDOW);
                                style |= (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
                                ::SetWindowLongPtr(actualHwnd, GWL_STYLE, style);

                                LONG_PTR exStyle = ::GetWindowLongPtr(actualHwnd, GWL_EXSTYLE);
                                exStyle &= ~WS_EX_APPWINDOW;
                                exStyle |= WS_EX_NOPARENTNOTIFY;
                                ::SetWindowLongPtr(actualHwnd, GWL_EXSTYLE, exStyle);

                                MakeDockable(actualHwnd, hostParent);
                            }
                            else
                            {
                                ::SetParent(actualHwnd, nullptr);
                            }

                            ::ShowWindow(actualHwnd, SW_SHOW);

                            w->hwnd = actualHwnd;
                            w->hdc = ctx->hdc;
                            w->glContext = ctx->hglrc;
                            w->context = ctx;
                            w->usesSharedContext = false;
                            w->running = true;
                            ctx->windowData = w;

                            RECT client{};
                            if (::GetClientRect(actualHwnd, &client))
                            {
                                const int actualWidth = clamp_positive(static_cast<int>(client.right - client.left));
                                const int actualHeight = clamp_positive(static_cast<int>(client.bottom - client.top));
                                w->width = actualWidth;
                                w->height = actualHeight;
                                ctx->width = actualWidth;
                                ctx->height = actualHeight;
                            }

                            if (previous && previous != actualHwnd)
                            {
                                auto& threads = Threads();
                                if (threads.contains(previous))
                                {
                                    auto node = threads.extract(previous);
                                    if (!node.empty())
                                    {
                                        node.key() = actualHwnd;
                                        threads.insert(std::move(node));
                                    }
                                }
                                ::DestroyWindow(previous);
                            }
                        };

                    // ---------------- Immediate backend initialization ----------------
                    switch (type)
                    {
#if defined(ALMOND_USING_OPENGL)
                    case ContextType::OpenGL:
                        if (ctx->hdc && ctx->hglrc)
                        {
                            if (!::wglMakeCurrent(ctx->hdc, ctx->hglrc))
                            {
                                std::cerr << "[Init] wglMakeCurrent failed for hwnd=" << hwnd << "\n";
                            }
                            else
                            {
                                std::cerr << "[Init] Running OpenGL init for hwnd=" << hwnd << "\n";
                                almondnamespace::openglcontext::opengl_initialize(ctx, hwnd, ctx->width, ctx->height, w ? w->onResize : nullptr);
                                ::wglMakeCurrent(nullptr, nullptr);
                            }
                        }
                        break;
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
                    case ContextType::Software:
                        std::cerr << "[Init] Initializing Software renderer for hwnd=" << hwnd << "\n";
                        almondnamespace::anativecontext::softrenderer_initialize(ctx, hwnd, ctx->width, ctx->height, w ? w->onResize : nullptr);
                        break;
#endif
#if defined(ALMOND_USING_SDL)
                    case ContextType::SDL:
                        std::cerr << "[Init] Initializing SDL context for hwnd=" << hwnd << "\n";
                        almondnamespace::sdlcontext::sdl_initialize(ctx, hostParent, ctx->width, ctx->height, w ? w->onResize : nullptr, narrowTitle);
                        break;
#endif
#if defined(ALMOND_USING_RAYLIB)
                    case ContextType::RayLib:
                        std::cerr << "[Init] Initializing RayLib context for hwnd=" << hwnd << "\n";
                        almondnamespace::raylibcontext::raylib_initialize(ctx, hostParent, ctx->width, ctx->height, w ? w->onResize : nullptr, narrowTitle);
                        break;
#endif
                    default:
                        break;
                    }

                    // Some backends (SDL/Raylib) create their own HWND; adopt it.
                    if (type == ContextType::SDL || type == ContextType::RayLib)
                        adopt_backend_window(ctx->hwnd);
                }
            };

#if defined(ALMOND_USING_RAYLIB)
        make_backend_windows(ContextType::RayLib, RayLibWinCount);
#endif
#if defined(ALMOND_USING_SDL)
        make_backend_windows(ContextType::SDL, SDLWinCount);
#endif
#if defined(ALMOND_USING_OPENGL)
        make_backend_windows(ContextType::OpenGL, OpenGLWinCount);
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
        make_backend_windows(ContextType::Software, SoftwareWinCount);
#endif
        (void)SFMLWinCount;

        ArrangeDockedWindowsGrid();

        {
            std::shared_lock lock(g_backendsMutex);
            return !g_backends.empty();
        }
    }

    void MultiContextManager::AddWindow(
        HWND hwnd,
        HWND parentWnd,
        HDC hdc,
        HGLRC glContext,
        bool usesSharedContext,
        ResizeCallback onResize,
        ContextType type)
    {
        if (!hwnd) return;

        s_activeInstance = this;

        if (!hdc) hdc = ::GetDC(hwnd);

#if defined(ALMOND_USING_OPENGL)
        if (type == ContextType::OpenGL && !glContext)
        {
            glContext = CreateSharedGLContext(hdc);
            static bool gladInitialized = false;
            if (!gladInitialized)
            {
                ::wglMakeCurrent(hdc, glContext);
                gladInitialized = (gladLoadGL() != 0);
                ::wglMakeCurrent(nullptr, nullptr);
            }
        }
#endif

        if (parentWnd) MakeDockable(hwnd, parentWnd);

        {
            bool needInit = false;
            {
                std::shared_lock lock(g_backendsMutex);
                needInit = g_backends.empty();
            }
            if (needInit) InitializeAllContexts();
        }

        std::shared_ptr<Context> ctx;
        {
            std::unique_lock lock(g_backendsMutex);
            auto& state = core::g_backends[type];

            if (!state.master)
            {
                ctx = std::make_shared<Context>();
                ctx->type = type;
                state.master = ctx;
            }
            else if (!state.master->windowData)
            {
                ctx = state.master;
            }
            else
            {
                auto it = std::find_if(state.duplicates.begin(), state.duplicates.end(),
                    [](const std::shared_ptr<Context>& dup) { return dup && !dup->windowData; });

                if (it != state.duplicates.end()) ctx = *it;
                else
                {
                    auto dup = CloneContext(*state.master);
                    state.duplicates.push_back(dup);
                    ctx = dup;
                }
            }
        }

        if (!ctx) return;

        ctx->type = type;
        ctx->hwnd = hwnd;
        ctx->hdc = hdc;
        ctx->hglrc = glContext;

        auto winPtr = std::make_unique<WindowData>(hwnd, hdc, glContext, usesSharedContext, type);
        winPtr->running = true;
        winPtr->onResize = std::move(onResize);
        winPtr->context = ctx;
        ctx->windowData = winPtr.get();

        RECT rc{};
        ::GetClientRect(hwnd, &rc);
        ctx->width = clamp_positive(static_cast<int>(rc.right - rc.left));
        ctx->height = clamp_positive(static_cast<int>(rc.bottom - rc.top));
        winPtr->width = ctx->width;
        winPtr->height = ctx->height;

        if (!ctx->onResize && winPtr->onResize) ctx->onResize = winPtr->onResize;
        if (!winPtr->onResize && ctx->onResize) winPtr->onResize = ctx->onResize;

        WindowData* rawWin = winPtr.get();
        {
            std::scoped_lock lock(windowsMutex);
            windows.emplace_back(std::move(winPtr));
        }

        auto& threads = Threads();
        if (!threads.contains(hwnd) && rawWin)
        {
            threads[hwnd] = std::thread([this, rawWin]() { RenderLoop(*rawWin); });
        }

        ArrangeDockedWindowsGrid();
    }

    void MultiContextManager::HandleResize(HWND hwnd, int width, int height)
    {
        if (!hwnd) return;

        int clampedWidth = clamp_positive(width);
        int clampedHeight = clamp_positive(height);
        backend::ResolveClientSize(hwnd, clampedWidth, clampedHeight);

        std::function<void(int, int)> resizeCallback;
        WindowData* window = nullptr;

        {
            std::scoped_lock lock(windowsMutex);
            auto it = std::find_if(windows.begin(), windows.end(),
                [hwnd](const std::unique_ptr<WindowData>& w) { return w && w->hwnd == hwnd; });
            if (it == windows.end()) return;

            window = it->get();
            window->width = clampedWidth;
            window->height = clampedHeight;

            if (window->context)
            {
                window->context->width = clampedWidth;
                window->context->height = clampedHeight;
                if (window->context->onResize) resizeCallback = window->context->onResize;
            }

            if (!resizeCallback && window->onResize) resizeCallback = window->onResize;
        }

        if (resizeCallback && window)
        {
            window->commandQueue.enqueue([
                cb = std::move(resizeCallback),
                w = clampedWidth,
                h = clampedHeight
            ]() mutable
                {
                    if (cb) cb(w, h);
                });
        }
    }

    void MultiContextManager::StartRenderThreads()
    {
        std::vector<HWND> hwnds;
        {
            std::scoped_lock lock(windowsMutex);
            hwnds.reserve(windows.size());
            for (const auto& w : windows)
                if (w && w->hwnd) hwnds.push_back(w->hwnd);
        }

        auto& threads = Threads();

        for (HWND hwnd : hwnds)
        {
            if (threads.contains(hwnd)) continue;

            threads[hwnd] = std::thread([this, hwnd]()
                {
                    WindowData* win = nullptr;
                    {
                        std::scoped_lock lock(windowsMutex);
                        auto it = std::find_if(windows.begin(), windows.end(),
                            [hwnd](const std::unique_ptr<WindowData>& w) { return w && w->hwnd == hwnd; });
                        if (it != windows.end()) win = it->get();
                    }

                    if (win) RenderLoop(*win);
                });
        }
    }

    void MultiContextManager::RemoveWindow(HWND hwnd)
    {
        std::unique_ptr<WindowData> removed;

        {
            std::scoped_lock lock(windowsMutex);
            auto it = std::find_if(windows.begin(), windows.end(),
                [hwnd](const std::unique_ptr<WindowData>& w) { return w && w->hwnd == hwnd; });
            if (it == windows.end()) return;

            (*it)->running = false;

            if ((*it)->context)
            {
                auto ctx = (*it)->context;
                if (ctx->windowData == it->get()) ctx->windowData = nullptr;
                if (ctx->hwnd == hwnd)
                {
                    ctx->hwnd = nullptr;
                    ctx->hdc = nullptr;
                    ctx->hglrc = nullptr;
                }
            }

            removed = std::move(*it);
            windows.erase(it);
        }

        auto& threads = Threads();
        if (threads.contains(hwnd))
        {
            if (threads[hwnd].joinable()) threads[hwnd].join();
            threads.erase(hwnd);
        }

#if defined(ALMOND_USING_OPENGL)
        if (removed && removed->glContext)
        {
            ::wglMakeCurrent(nullptr, nullptr);
            ::wglDeleteContext(removed->glContext);
        }
#endif
        if (removed && removed->hdc && removed->hwnd)
            ::ReleaseDC(removed->hwnd, removed->hdc);
    }

    void MultiContextManager::ArrangeDockedWindowsGrid()
    {
        if (!parent) return;

        std::scoped_lock lock(windowsMutex);
        if (windows.empty()) return;

        const int total = static_cast<int>(windows.size());
        int cols = 1, rows = 1;
        while (cols * rows < total) (cols <= rows ? ++cols : ++rows);

        RECT rcClient{};
        ::GetClientRect(parent, &rcClient);
        const int clientW = rcClient.right - rcClient.left;
        const int clientH = rcClient.bottom - rcClient.top;

        const int cw = clamp_positive(clientW / cols);
        const int ch = clamp_positive(clientH / rows);

        for (size_t i = 0; i < windows.size(); ++i)
        {
            const int c = static_cast<int>(i) % cols;
            const int r = static_cast<int>(i) / cols;

            WindowData& win = *windows[i];
            ::SetWindowPos(win.hwnd, nullptr, c * cw, r * ch, cw, ch,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

            // Queue resize into that window's render queue.
            HandleResize(win.hwnd, cw, ch);
        }
    }

    void MultiContextManager::StopAll()
    {
        running.store(false, std::memory_order_release);

        {
            std::scoped_lock lock(windowsMutex);
            for (auto& w : windows)
                if (w) w->running = false;
        }

        for (auto& [hwnd, th] : g_threads)
        {
            if (th.joinable()) th.join();
        }
        g_threads.clear();

        if (s_activeInstance == this) s_activeInstance = nullptr;
    }

    void MultiContextManager::RenderLoop(WindowData& win)
    {
        auto ctx = win.context;
        if (!ctx)
        {
            win.running = false;
            return;
        }

        ctx->windowData = &win;
        SetCurrent(ctx);

        struct ResetGuard { ~ResetGuard() { MultiContextManager::SetCurrent(nullptr); } } resetGuard;

        if (ctx->initialize) ctx->initialize_safe();
        else
        {
            win.running = false;
            return;
        }

        while (running.load(std::memory_order_acquire) && win.running)
        {
            bool keepRunning = true;

            if (ctx->process) keepRunning = ctx->process_safe(ctx, win.commandQueue);
            else win.commandQueue.drain();

            if (!keepRunning)
            {
                win.running = false;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        if (ctx->cleanup) ctx->cleanup_safe();
    }

    void MultiContextManager::HandleDropFiles(HWND, HDROP hDrop)
    {
        const UINT count = ::DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; ++i)
        {
            wchar_t path[MAX_PATH]{};
            ::DragQueryFileW(hDrop, i, path, MAX_PATH);
            std::wcout << L"[Drop] " << path << L"\n";
        }
    }

    LRESULT CALLBACK MultiContextManager::ParentProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return TRUE;
        }

        auto* mgr = reinterpret_cast<MultiContextManager*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!mgr) return ::DefWindowProcW(hwnd, msg, wParam, lParam);

        switch (msg)
        {
        case WM_SIZE:
            mgr->ArrangeDockedWindowsGrid();
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = ::BeginPaint(hwnd, &ps);
            ::FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            ::EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DROPFILES:
            mgr->HandleDropFiles(hwnd, reinterpret_cast<HDROP>(wParam));
            ::DragFinish(reinterpret_cast<HDROP>(wParam));
            return 0;

        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            if (hwnd == mgr->GetParentWindow()) ::PostQuitMessage(0);
            return 0;
        }

        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK MultiContextManager::ChildProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        static DragState& drag = Drag();

        if (msg == WM_NCCREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return TRUE;
        }

        switch (msg)
        {
        case WM_LBUTTONDOWN:
        {
            ::SetCapture(hwnd);
            drag.dragging = true;
            drag.draggedWindow = hwnd;
            drag.originalParent = ::GetParent(hwnd);
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ::ClientToScreen(hwnd, &pt);
            drag.lastMousePos = pt;
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (!drag.dragging || drag.draggedWindow != hwnd)
                return ::DefWindowProcW(hwnd, msg, wParam, lParam);

            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ::ClientToScreen(hwnd, &pt);

            const int dx = pt.x - drag.lastMousePos.x;
            const int dy = pt.y - drag.lastMousePos.y;
            drag.lastMousePos = pt;

            RECT wndRect{};
            ::GetWindowRect(hwnd, &wndRect);

            const int newX = wndRect.left + dx;
            const int newY = wndRect.top + dy;

            if (drag.originalParent)
            {
                RECT prc{};
                ::GetClientRect(drag.originalParent, &prc);
                POINT tl{ 0,0 };
                ::ClientToScreen(drag.originalParent, &tl);
                ::OffsetRect(&prc, tl.x, tl.y);

                const int wndW = wndRect.right - wndRect.left;
                const int wndH = wndRect.bottom - wndRect.top;

                const bool inside =
                    newX >= prc.left && newY >= prc.top &&
                    (newX + wndW) <= prc.right && (newY + wndH) <= prc.bottom;

                if (inside)
                {
                    if (::GetParent(hwnd) != drag.originalParent)
                    {
                        LONG_PTR style = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
                        style &= ~(WS_POPUP | WS_OVERLAPPEDWINDOW);
                        style |= WS_CHILD;
                        ::SetWindowLongPtrW(hwnd, GWL_STYLE, style);
                        ::SetParent(hwnd, drag.originalParent);

                        POINT cp{ newX, newY };
                        ::ScreenToClient(drag.originalParent, &cp);
                        ::SetWindowPos(hwnd, nullptr, cp.x, cp.y, wndW, wndH,
                            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                    }
                    else
                    {
                        POINT cp{ newX, newY };
                        ::ScreenToClient(drag.originalParent, &cp);
                        ::SetWindowPos(hwnd, nullptr, cp.x, cp.y, 0, 0,
                            SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
                    }
                }
                else
                {
                    if (::GetParent(hwnd) == drag.originalParent)
                    {
                        LONG_PTR style = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
                        style &= ~WS_CHILD;
                        style |= WS_OVERLAPPEDWINDOW | WS_VISIBLE;
                        ::SetWindowLongPtrW(hwnd, GWL_STYLE, style);
                        ::SetParent(hwnd, nullptr);
                        ::SetWindowPos(hwnd, nullptr, newX, newY, wndW, wndH,
                            SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                    }
                    else
                    {
                        ::SetWindowPos(hwnd, nullptr, newX, newY, 0, 0,
                            SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
                    }
                }
            }
            else
            {
                ::SetWindowPos(hwnd, nullptr, newX, newY, 0, 0,
                    SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            }

            return 0;
        }

        case WM_SIZE:
        {
            if (wParam == SIZE_MINIMIZED) return 0;

            auto* mgr = s_activeInstance;
            if (!mgr) break;

            const int width = clamp_positive(static_cast<int>(LOWORD(lParam)));
            const int height = clamp_positive(static_cast<int>(HIWORD(lParam)));
            mgr->HandleResize(hwnd, width, height);
            return 0;
        }

        case WM_LBUTTONUP:
            if (drag.dragging && drag.draggedWindow == hwnd)
            {
                ::ReleaseCapture();
                drag.dragging = false;
                drag.draggedWindow = nullptr;
                drag.originalParent = nullptr;
            }
            return 0;

        case WM_DROPFILES:
            if (HWND p = ::GetParent(hwnd))
                ::SendMessageW(p, WM_DROPFILES, wParam, lParam);
            ::DragFinish(reinterpret_cast<HDROP>(wParam));
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = ::BeginPaint(hwnd, &ps);
            ::FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            ::EndPaint(hwnd, &ps);
            return 0;
        }

        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        return 0;
    }
}

#endif // _WIN32
