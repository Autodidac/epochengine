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
// araylibcontext.hpp
#pragma once

#include "aengineconfig.hpp"

#if defined(ALMOND_USING_RAYLIB)


#include "acontext.hpp"
#include "awindowdata.hpp"

#include "acontextwindow.hpp"
#include "arobusttime.hpp"
#include "aimageloader.hpp"
#include "araylibtextures.hpp" // AtlasGPU, gpu_atlases, ensure_uploaded
#include "araylibstate.hpp" // brings in the actual inline s_state
#include "aatlasmanager.hpp"
#include "araylibrenderer.hpp" // RendererContext, RenderMode
#include "acommandline.hpp" // cli::window_width, cli::window_height
#include "araylibcontextinput.hpp" // poll_input()

//#include "araylibcontext_win32.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>

// FOR EXAMPLE
//#include <windows.h>
// Raylib includes 
#include <raylib.h> // Ensure this is included after platform-specific headers

namespace almondnamespace::raylibcontext
{
    // Usually at the start of your program, before window creation:
    static almondnamespace::contextwindow::WindowData* g_raylibwindowContext;

    inline void dispatch_resize(const std::shared_ptr<core::Context>& ctx,
        unsigned int width,
        unsigned int height,
        bool updateRaylibWindow,
        bool notifyClient = true)
    {
        unsigned int nextWidth = width;
        unsigned int nextHeight = height;
        bool nextUpdateWindow = updateRaylibWindow;
        bool nextNotifyClient = notifyClient;

        if (s_raylibstate.dispatchingResize)
        {
            s_raylibstate.pendingWidth = nextWidth;
            s_raylibstate.pendingHeight = nextHeight;
            s_raylibstate.pendingUpdateWindow = s_raylibstate.pendingUpdateWindow || nextUpdateWindow;
            s_raylibstate.pendingNotifyClient = s_raylibstate.pendingNotifyClient || nextNotifyClient;
            s_raylibstate.hasPendingResize = true;
            return;
        }

        s_raylibstate.dispatchingResize = true;

        for (;;)
        {
            const unsigned int safeWidth = std::max(1u, nextWidth);
            const unsigned int safeHeight = std::max(1u, nextHeight);

            s_raylibstate.width = safeWidth;
            s_raylibstate.height = safeHeight;

            if (ctx)
            {
                ctx->width = safeWidth;
                ctx->height = safeHeight;
            }

#if !defined(RAYLIB_NO_WINDOW)
            if (nextUpdateWindow && IsWindowReady())
            {
                SetWindowSize(static_cast<int>(safeWidth), static_cast<int>(safeHeight));
            }
#endif

            if (nextNotifyClient && s_raylibstate.clientOnResize)
            {
                try
                {
                    s_raylibstate.clientOnResize(static_cast<int>(safeWidth), static_cast<int>(safeHeight));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "[Raylib] onResize client callback threw: " << e.what() << "\n";
                }
                catch (...)
                {
                    std::cerr << "[Raylib] onResize client callback threw unknown exception.\n";
                }
            }

            if (!s_raylibstate.hasPendingResize)
            {
                break;
            }

            nextWidth = s_raylibstate.pendingWidth;
            nextHeight = s_raylibstate.pendingHeight;
            nextUpdateWindow = s_raylibstate.pendingUpdateWindow;
            nextNotifyClient = s_raylibstate.pendingNotifyClient;

            s_raylibstate.hasPendingResize = false;
            s_raylibstate.pendingUpdateWindow = false;
            s_raylibstate.pendingNotifyClient = false;
        }

        s_raylibstate.dispatchingResize = false;
    }

   // HWND hwnd;
    //struct RaylibState {
    //    HWND parent{};
    //    HWND hwnd{};
    //    HDC  hdc{};       // Store device context
    //    HGLRC glContext{}; // Store GL context created by Raylib
    //    bool running{ false };
    //    unsigned int width{ 800 };
    //    unsigned int height{ 600 };
    //    std::function<void(int, int)> onResize;
    //};
    //inline RaylibState raylibcontext;
    
    // ──────────────────────────────────────────────
    // Initialize Raylib window and context
    // ──────────────────────────────────────────────
    inline bool raylib_initialize(std::shared_ptr<core::Context> ctx, HWND parentWnd = nullptr, unsigned int w = 800, unsigned int h = 600, std::function<void(int, int)> onResize = nullptr)
    {
        const unsigned int clampedWidth = std::max(1u, w);
        const unsigned int clampedHeight = std::max(1u, h);

        s_raylibstate.clientOnResize = std::move(onResize);
        s_raylibstate.parent = parentWnd;
        s_raylibstate.dispatchingResize = false;
        s_raylibstate.hasPendingResize = false;
        s_raylibstate.pendingUpdateWindow = false;
        s_raylibstate.pendingNotifyClient = false;
        s_raylibstate.pendingWidth = 0;
        s_raylibstate.pendingHeight = 0;

        std::weak_ptr<core::Context> ctxWeak = ctx;
        s_raylibstate.onResize = [ctxWeak](int width, int height)
        {
            const int safeWidth = std::max(1, width);
            const int safeHeight = std::max(1, height);
            auto locked = ctxWeak.lock();
            dispatch_resize(locked,
                static_cast<unsigned int>(safeWidth),
                static_cast<unsigned int>(safeHeight),
                true);
        };

        if (ctx)
        {
            ctx->onResize = s_raylibstate.onResize;
        }

        dispatch_resize(ctx, clampedWidth, clampedHeight, false);


        static bool initialized = false;
        if (initialized) return true;
        initialized = true;


        auto& backend = almondnamespace::raylibtextures::get_raylib_backend();
        auto& rlState = backend.rlState;

        // -----------------------------------------------------------------
        // Step 1: Resolve window/context handles
        // -----------------------------------------------------------------
        HWND   resolvedHwnd = nullptr;
        HDC    resolvedHdc = nullptr;
        HGLRC  resolvedHglrc = nullptr;

        if (ctx && ctx->windowData) {
            resolvedHwnd = ctx->windowData->hwnd;
            resolvedHdc = ctx->windowData->hdc;
            resolvedHglrc = ctx->windowData->glContext;
            std::cerr << "[Raylib Init] Using ctx->windowData handles\n";
        }

        // Fallbacks: pull directly from current thread
        if (!resolvedHdc)   resolvedHdc = wglGetCurrentDC();
        if (!resolvedHglrc) resolvedHglrc = wglGetCurrentContext();

        // Parent HWND priority
        HWND resolvedParent = parentWnd ? parentWnd : resolvedHwnd;

        // Assign hwnd only if we actually resolved one
        if (resolvedHwnd) {
            rlState.hwnd = resolvedHwnd;
            std::cerr << "[Raylib Init] Using resolved hwnd=" << resolvedHwnd << "\n";
        }
        else if (parentWnd) {
            rlState.hwnd = parentWnd;
            std::cerr << "[Raylib Init] Using parent hwnd=" << parentWnd << "\n";
        }
        else {
            // Leave glState.hwnd unchanged if it was already set earlier
            std::cerr << "[Raylib Init] WARNING: No hwnd resolved; keeping existing rlState.hwnd="
                << rlState.hwnd << "\n";
        }


        InitWindow(s_raylibstate.width, s_raylibstate.height, "Raylib Window");
        // SetTargetFPS(60);  // Cap FPS to something sane

        s_raylibstate.hwnd = (HWND)GetWindowHandle();
        s_raylibstate.hdc = GetDC(s_raylibstate.hwnd);
        s_raylibstate.glContext = wglGetCurrentContext();  // Get Raylib's GL context _immediately_ after InitWindow

        std::cout << "[Raylib] Context: " << s_raylibstate.glContext << "\n";

        if (s_raylibstate.parent) {
            RECT parentRect;
            GetWindowRect(s_raylibstate.parent, &parentRect);
            SetParent(s_raylibstate.hwnd, s_raylibstate.parent);
            ShowWindow(s_raylibstate.hwnd, SW_SHOW);

            LONG_PTR style = GetWindowLongPtr(s_raylibstate.hwnd, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= WS_CHILD | WS_VISIBLE;
            SetWindowLongPtr(s_raylibstate.hwnd, GWL_STYLE, style);

            RECT client{};
            GetClientRect(s_raylibstate.parent, &client);
            const int width = std::max<LONG>(1, client.right - client.left);
            const int height = std::max<LONG>(1, client.bottom - client.top);

            s_raylibstate.width = width;
            s_raylibstate.height = height;

            SetWindowPos(s_raylibstate.hwnd, nullptr, 0, 0,
                width, height,
                SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

#if !defined(RAYLIB_NO_WINDOW)
            if (IsWindowReady()) {
                SetWindowSize(width, height);
            }
#endif

            if (s_raylibstate.onResize) {
                s_raylibstate.onResize(width, height);
            }

            PostMessage(s_raylibstate.parent, WM_SIZE, 0, MAKELPARAM(width, height));
        }

        s_raylibstate.running = true;

        atlasmanager::register_backend_uploader(core::ContextType::RayLib,
            [](const TextureAtlas& atlas) {
                raylibtextures::ensure_uploaded(atlas);
            });

        return true;

//        //InitRaylibWindow();
//        // 1. Ensure a context exists
//        //if (!almondnamespace::ContextManager::hasContext()) {
//        //    almondnamespace::ContextManager::setContext(std::make_shared<almondnamespace::state::ContextState>());
//        //}
//
//        // Optional: you can sync these to your context window module
//       // s_state.screenWidth = s_state.window.get_width()
//       // s_state.screenHeight = s_state.window.get_height()
//
//        InitWindow(s_raylibstate.screenWidth, s_raylibstate.screenHeight, "AlmondEngine - Raylib");
//#if defined(_WIN32)
////        s_raylibstate.hwnd = (HWND)GetWindowHandle();
//        //HWND raylibHwnd = (HWND)GetWindowHandle();
//       // s_raylibstate.hwnd = raylibHwnd; // Store for your engine
//	//	DockRaylibWindow(raylibHwnd);
//
//        // Allocate and configure your context
//        auto* winCtx = new almondnamespace::contextwindow::WindowData();
//        //winCtx->setWindowHandle((HWND)GetWindowHandle()); // Raylib's HWND
//        almondnamespace::contextwindow::WindowData::set_global_instance(winCtx);
//        HWND hwndParent = winCtx->getWindowHandle();
//       // winCtx->setParentHandle(hwndParent);
//        winCtx->setParentHandle(hwndParent);
//       // almondnamespace::contextwindow::WindowData::setParentHandle(hwndParent);
//        winCtx->setChildHandle((HWND)GetWindowHandle()); // Set this if you want to dock Raylib window into a parent window
//        HWND hwndChild = winCtx ->getChildHandle();
//        
//        //core::create_window();
//        // 4. Optionally adjust window style for child behavior
//        //POINT cursor;
//        //GetCursorPos(&cursor);
//
//        //RECT parentRect;
//        //GetWindowRect(hwndParent, &parentRect);
//        ////if (PtInRect(&parentRect, cursor)) {
//        LONG_PTR style = GetWindowLongPtr(hwndChild, GWL_STYLE);
//        style &= ~WS_OVERLAPPED;
//        style |= WS_CHILD | WS_VISIBLE;
//        SetWindowLongPtr(hwndChild, GWL_STYLE, style);
//        SetParent(hwndChild, hwndParent);
//        SetWindowPos(hwndChild, nullptr, 0, 0, core::cli::window_width, core::cli::window_height, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
//
//        RECT rc;
//        GetClientRect(hwndParent, &rc);
//        PostMessage(hwndParent, WM_SIZE, 0, MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));
//
//        //almondnamespace::core::create_window(hi, rc.right - rc.left, rc.bottom - rc.top, L"AlmondChild", L"Child", hwndParent);
//
//        //hwnd = (HWND)GetWindowHandle();
//        //almondnamespace::contextwindow::WindowContext::set_global_instance(g_raylibwindowContext);
//        // Dock Raylib window into parent if requested (Win32 only, not officially supported by Raylib)
//        //if (parentHwnd && s_raylibstate.hwnd) {
//           // SetParent(s_raylibstate.hwnd, parentHwnd);
//            // Optionally adjust window style here if needed
//       // }
//       // s_raylibstate.hwnd = contextwindow::WindowContext::getWindowHandle();
//#endif
//
//
//        // You can enable VSync or multi-sampling etc.
//        //SetTargetFPS(60);
//        return true;
    }

    inline void Raylib_CloseWindow() { 
        //::CloseWindow(s_raylibstate.hwnd);
//        if (hwnd)
 //           PostMessage(hwnd, WM_CLOSE, 0, 0); // ask window to close gracefully
    }
    // ──────────────────────────────────────────────
    // Per-frame event processing
    // ──────────────────────────────────────────────
    inline bool raylib_process(std::shared_ptr<core::Context> ctx, core::CommandQueue& queue) {
        (void)ctx;
        if (!s_raylibstate.running || WindowShouldClose()) {
            s_raylibstate.running = false;
            return true;
        }

        atlasmanager::process_pending_uploads(core::ContextType::RayLib);

        const unsigned int previousWidth = s_raylibstate.width;
        const unsigned int previousHeight = s_raylibstate.height;

        unsigned int observedWidth = previousWidth;
        unsigned int observedHeight = previousHeight;

        const int currentWidth = GetScreenWidth();
        const int currentHeight = GetScreenHeight();
        if (currentWidth > 0) {
            observedWidth = static_cast<unsigned int>(currentWidth);
        }
        if (currentHeight > 0) {
            observedHeight = static_cast<unsigned int>(currentHeight);
        }

        if (observedWidth != previousWidth || observedHeight != previousHeight) {
            dispatch_resize(ctx, observedWidth, observedHeight, false);
        }

        if (!wglMakeCurrent(s_raylibstate.hdc, s_raylibstate.glContext)) {
            s_raylibstate.running = false;
            std::cerr << "[Raylib] Failed to make Raylib GL context current\n";
            return true;
        }
        //std::cout << "Running! " << '\n';
        // inside your raylib process function
        static auto* bgTimer = almondnamespace::time::getTimer("menu", "bg_color");
        if (!bgTimer)
            bgTimer = &almondnamespace::time::createNamedTimer("menu", "bg_color");

        double t = almondnamespace::time::elapsed(*bgTimer); // seconds

        // Map sin output [−1,1] to [0,255] for Raylib Color
        unsigned char r = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 1.0)) * 255);
        unsigned char g = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 0.7 + 2.0)) * 255);
        unsigned char b = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 1.3 + 4.0)) * 255);


        ClearBackground(Color{ r, g, b, 255 });
        BeginDrawing();

        queue.drain();

        EndDrawing();

        // Optional: unbind context if you want to be neat
        // wglMakeCurrent(nullptr, nullptr);
        //almondnamespace::s_raylibstate::poll_input();
        //s_raylibstate.shouldClose = WindowShouldClose();
        //return !s_raylibstate.shouldClose;
		return true; // Continue running
    }

    // ──────────────────────────────────────────────
    // Clear the screen — Raylib style
    // ──────────────────────────────────────────────
    inline void raylib_clear()
    {
        BeginDrawing();
        ClearBackground(DARKPURPLE);
    }

    // ──────────────────────────────────────────────
    // Present your frame — Raylib style
    // ──────────────────────────────────────────────
    inline void raylib_present()
    {
        EndDrawing();
    }

    // ──────────────────────────────────────────────
    // Cleanup and shutdown
    // ──────────────────────────────────────────────
    inline void raylib_cleanup(std::shared_ptr<almondnamespace::core::Context>& ctx) {
        if (!s_raylibstate.running) return;
        CloseWindow(s_raylibstate.hwnd);  // Raylib manages its window internally
        s_raylibstate.running = false;
       // Raylib_CloseWindow();
    }

    // ──────────────────────────────────────────────
    // Extra helpers if you like consistency
    // ──────────────────────────────────────────────
    inline int raylib_get_width()  noexcept { return static_cast<int>(s_raylibstate.width); }
    inline int raylib_get_height() noexcept { return static_cast<int>(s_raylibstate.height); }
    inline void raylib_set_window_title(const std::string& title)
    {
        SetWindowTitle(title.c_str());
    }

    inline bool RaylibIsRunning(std::shared_ptr<core::Context> ctx) {
        return s_raylibstate.running;
    }
} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
