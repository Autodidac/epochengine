module;

#ifndef SDL_WINDOW_FULLSCREEN_DESKTOP
#define SDL_WINDOW_FULLSCREEN_DESKTOP 1
#endif

#define SDL_MAIN_HANDLED
#include "aengine.config.hpp" 		// for ALMOND_USING_SDL

export module acontext.sdl.context;

//import aengine.config;
import aengine.platform;

import <chrono>;
import <cstdint>;
import <algorithm>;
import <functional>;
import <memory>;
import <mutex>;
import <stdexcept>;
import <iostream>;
import <string>;
import <SDL3/SDL.h>;

import aengine.core.context;
import aengine.context.window;
import aengine.context.commandqueue;
import aengine.context.control;
import aatlas.manager;
import acontext.sdl.state;
import acontext.sdl.renderer;
import acontext.sdl.textures;
import aengine.context.multiplexer; // almondnamespace::core::MakeDockable(sdlcontext.hwnd, sdlcontext.parent);
import aengine.telemetry;



export namespace almondnamespace::sdlcontext
{
#if defined(ALMOND_USING_SDL)
    struct SDLState {
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;

        HWND parent = nullptr;
        HWND hwnd = nullptr;
        bool running = false;
        int width = 400;
        int height = 300;
        int framebufferWidth = 400;
        int framebufferHeight = 300;
        int virtualWidth = 400;
        int virtualHeight = 300;
        std::function<void(int, int)> onResize;
    };

    inline SDLState sdlcontext;

    inline void refresh_dimensions(const std::shared_ptr<core::Context>& ctx) noexcept
    {
        int logicalW = (std::max)(1, sdlcontext.width);
        int logicalH = (std::max)(1, sdlcontext.height);

        if (sdlcontext.window)
        {
            int windowW = 0;
            int windowH = 0;
            SDL_GetWindowSize(sdlcontext.window, &windowW, &windowH);
            if (windowW > 0 && windowH > 0)
            {
                logicalW = windowW;
                logicalH = windowH;
            }
        }

        sdlcontext.width = logicalW;
        sdlcontext.height = logicalH;
        sdlcontext.virtualWidth = logicalW;
        sdlcontext.virtualHeight = logicalH;

        int fbW = logicalW;
        int fbH = logicalH;
        if (sdlcontext.renderer)
        {
            int renderW = 0;
            int renderH = 0;
            if (SDL_GetCurrentRenderOutputSize(sdlcontext.renderer, &renderW, &renderH) == 0
                && renderW > 0 && renderH > 0)
            {
                fbW = renderW;
                fbH = renderH;
            }
        }

        sdlcontext.framebufferWidth = (std::max)(1, fbW);
        sdlcontext.framebufferHeight = (std::max)(1, fbH);

        if (ctx)
        {
            ctx->width = sdlcontext.width;
            ctx->height = sdlcontext.height;
            ctx->virtualWidth = sdlcontext.virtualWidth;
            ctx->virtualHeight = sdlcontext.virtualHeight;
            ctx->framebufferWidth = sdlcontext.framebufferWidth;
            ctx->framebufferHeight = sdlcontext.framebufferHeight;
        }
    }

    inline bool sdl_initialize(std::shared_ptr<core::Context> ctx,
        HWND parentWnd = nullptr,
        int w = 400,
        int h = 300,
        std::function<void(int, int)> onResize = nullptr,
        std::string windowTitle = {})
    {
        const int clampedWidth = (std::max)(1, w);
        const int clampedHeight = (std::max)(1, h);

        sdlcontext.width = clampedWidth;
        sdlcontext.height = clampedHeight;
        sdlcontext.virtualWidth = clampedWidth;
        sdlcontext.virtualHeight = clampedHeight;
        sdlcontext.framebufferWidth = clampedWidth;
        sdlcontext.framebufferHeight = clampedHeight;
        sdlcontext.parent = parentWnd;

        refresh_dimensions(ctx);

        std::weak_ptr<core::Context> weakCtx = ctx;
        auto userResize = std::move(onResize);
        sdlcontext.onResize = [weakCtx, userResize = std::move(userResize)](int width, int height) mutable {
            sdlcontext.width = (std::max)(1, width);
            sdlcontext.height = (std::max)(1, height);
            sdlcontext.virtualWidth = sdlcontext.width;
            sdlcontext.virtualHeight = sdlcontext.height;

            if (sdlcontext.window) {
                SDL_SetWindowSize(sdlcontext.window, sdlcontext.width, sdlcontext.height);
            }

            auto locked = weakCtx.lock();
            refresh_dimensions(locked);

            if (userResize) {
                userResize(sdlcontext.framebufferWidth, sdlcontext.framebufferHeight);
            }
        };

        if (ctx) {
            ctx->onResize = sdlcontext.onResize;
        }

        // SDL3 returns 0 on success, negative on failure.
        // Force the type with cast so MSVC can’t mis-deduce
        if (static_cast<int>(SDL_Init(SDL_INIT_VIDEO)) < 0) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }

        if (windowTitle.empty()) {
            windowTitle = "SDL3 Window";
        }

        // Create properties object
        SDL_PropertiesID props = SDL_CreateProperties();
        if (!props) {
            std::cerr << "[SDL] SDL_CreateProperties failed: " << SDL_GetError() << "\n";
            SDL_Quit();
            return false;
        }

        // Set window properties
        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, windowTitle.c_str());
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, sdlcontext.width);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, sdlcontext.height);
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

        // Create window with properties
        sdlcontext.window = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);

        if (!sdlcontext.window) {
            std::cerr << "[SDL] SDL_CreateWindowWithProperties failed: " << SDL_GetError() << "\n";
            SDL_Quit();
            return false;
        }

#if defined(_WIN32)
        SDL_PropertiesID windowProps = SDL_GetWindowProperties(sdlcontext.window);
        if (!windowProps) {
            std::cerr << "[SDL] SDL_GetWindowProperties failed: " << SDL_GetError() << "\n";
            SDL_DestroyWindow(sdlcontext.window);
            SDL_Quit();
            return false;
        }

        HWND hwnd = (HWND)SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        sdlcontext.hwnd = hwnd;
        if (!sdlcontext.hwnd) {
            std::cerr << "[SDL] Failed to retrieve HWND\n";
            SDL_DestroyWindow(sdlcontext.window);
            SDL_Quit();
            return false;
        }

        if (ctx) {
            ctx->hwnd = sdlcontext.hwnd;
        }
#else
        (void)parentWnd;
        if (ctx) {
            ctx->hwnd = nullptr;
        }
#endif

        SDL_SetWindowTitle(sdlcontext.window, windowTitle.c_str());

        // Create renderer
        sdlcontext.renderer = SDL_CreateRenderer(sdlcontext.window, nullptr);
        if (!sdlcontext.renderer) {
            std::cerr << "[SDL] SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
            SDL_DestroyWindow(sdlcontext.window);
            SDL_Quit();
            return false;
        }

        init_renderer(sdlcontext.renderer);
        sdltextures::sdl_renderer = sdlcontext.renderer;

        refresh_dimensions(ctx);

#if defined(_WIN32)
        if (sdlcontext.parent) {
            std::cout << "[SDL] Setting parent window: " << sdlcontext.parent << "\n";
            SetParent(sdlcontext.hwnd, sdlcontext.parent);

            LONG_PTR style = GetWindowLongPtr(sdlcontext.hwnd, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= WS_CHILD | WS_VISIBLE;
            SetWindowLongPtr(sdlcontext.hwnd, GWL_STYLE, style);

            almondnamespace::core::MakeDockable(sdlcontext.hwnd, sdlcontext.parent);

            if (!windowTitle.empty()) {
                const std::wstring wideTitle(windowTitle.begin(), windowTitle.end());
                SetWindowTextW(sdlcontext.hwnd, wideTitle.c_str());
            }

            RECT client{};
            GetClientRect(sdlcontext.parent, &client);
            const int width = static_cast<int>((std::max)(static_cast<LONG>(1), client.right - client.left));
            const int height = static_cast<int>((std::max)(static_cast<LONG>(1), client.bottom - client.top));

            sdlcontext.width = width;
            sdlcontext.height = height;

            SetWindowPos(sdlcontext.hwnd, nullptr, 0, 0,
                width, height,
                SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            if (sdlcontext.onResize) {
                sdlcontext.onResize(width, height);
            }

            PostMessage(sdlcontext.parent, WM_SIZE, 0, MAKELPARAM(width, height));
        }
#endif

        SDL_ShowWindow(sdlcontext.window);
        sdlcontext.running = true;

        atlasmanager::register_backend_uploader(core::ContextType::SDL,
            [](const TextureAtlas& atlas) {
                sdltextures::ensure_uploaded(atlas);
            });

        return true;
    }

    inline bool sdl_process(std::shared_ptr<core::Context> ctx, core::CommandQueue& queue) {
        const auto frameStart = std::chrono::steady_clock::now();
        const core::ContextType backendType = ctx ? ctx->type : core::ContextType::SDL;
        const std::uintptr_t windowId = ctx && ctx->windowData
            ? reinterpret_cast<std::uintptr_t>(ctx->windowData->hwnd)
            : reinterpret_cast<std::uintptr_t>(sdlcontext.hwnd);

        SDL_Event sdl_event{};
        while (SDL_PollEvent(&sdl_event)) {
            if (sdl_event.type == SDL_EVENT_QUIT) {
                sdlcontext.running = false;
                return false;
            }
            if (sdl_event.type == SDL_EVENT_WINDOW_RESIZED && sdlcontext.onResize) {
                sdlcontext.onResize(sdl_event.window.data1, sdl_event.window.data2);
            }
        }

        refresh_dimensions(ctx);

        telemetry::emit_gauge(
            "renderer.framebuffer.size",
            static_cast<std::int64_t>(sdlcontext.framebufferWidth),
            telemetry::RendererTelemetryTags{ backendType, windowId, "width" });
        telemetry::emit_gauge(
            "renderer.framebuffer.size",
            static_cast<std::int64_t>(sdlcontext.framebufferHeight),
            telemetry::RendererTelemetryTags{ backendType, windowId, "height" });

        SDL_SetRenderDrawColor(sdl_renderer.renderer, 124, 0, 255, 255);
        SDL_RenderClear(sdl_renderer.renderer);

        {
            std::size_t depth = 0;
            {
                std::scoped_lock lock(queue.get_mutex());
                depth = queue.get_queue().size();
            }
            telemetry::emit_gauge(
                "renderer.command_queue.depth",
                static_cast<std::int64_t>(depth),
                telemetry::RendererTelemetryTags{ backendType, windowId });
        }

        queue.drain();

        SDL_RenderPresent(sdl_renderer.renderer);
        const auto frameEnd = std::chrono::steady_clock::now();
        const auto frameMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
        telemetry::emit_histogram_ms(
            "renderer.frame.time_ms",
            frameMs,
            telemetry::RendererTelemetryTags{ backendType, windowId });
        return true;
    }

    inline void sdl_clear()
    {
        SDL_SetRenderDrawColor(sdl_renderer.renderer, 0, 0, 0, 255);
        SDL_RenderClear(sdl_renderer.renderer);
    }

    inline void sdl_present()
    {
        SDL_RenderPresent(sdl_renderer.renderer);
    }

    inline void sdl_cleanup(std::shared_ptr<almondnamespace::core::Context>& ctx) {
        if (sdlcontext.renderer) {
            SDL_DestroyRenderer(sdlcontext.renderer);
            sdlcontext.renderer = nullptr;
        }
        if (sdlcontext.window) {
            SDL_DestroyWindow(sdlcontext.window);
            sdlcontext.window = nullptr;
        }
        SDL_Quit();
        sdlcontext.running = false;
    }

    inline void set_window_position_centered()
    {
        if (state::get_sdl_state().window.sdl_window)
        {
            int w, h;
            SDL_GetWindowSize(state::get_sdl_state().window.sdl_window, &w, &h);
            SDL_SetWindowPosition(state::get_sdl_state().window.sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }

    inline void set_window_position(int x, int y)
    {
        if (state::get_sdl_state().window.sdl_window)
            SDL_SetWindowPosition(state::get_sdl_state().window.sdl_window, x, y);
    }
    inline void set_window_fullscreen(bool fullscreen)
    {
        if (state::get_sdl_state().window.sdl_window)
        {
            if (fullscreen)
                SDL_SetWindowFullscreen(state::get_sdl_state().window.sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            else
                SDL_SetWindowFullscreen(state::get_sdl_state().window.sdl_window, 0);
        }
    }
    inline void set_window_borderless(bool borderless)
    {
        if (state::get_sdl_state().window.sdl_window)
        {
            SDL_SetWindowBordered(state::get_sdl_state().window.sdl_window, !borderless);
        }
    }
    inline void set_window_is_resizable(bool resizable)
    {
        if (state::get_sdl_state().window.sdl_window)
        {
            if (resizable)
                SDL_SetWindowResizable(state::get_sdl_state().window.sdl_window, true);
            else
                SDL_SetWindowResizable(state::get_sdl_state().window.sdl_window, false);
        }
    }
    inline void set_window_minimized()
    {
        if (state::get_sdl_state().window.sdl_window)
        {
            SDL_MinimizeWindow(state::get_sdl_state().window.sdl_window);
        }
    }
    inline void set_window_size(int width, int height)
    {
        if (state::get_sdl_state().window.sdl_window)
            SDL_SetWindowSize(state::get_sdl_state().window.sdl_window, width, height);
        }

    inline void sdl_set_window_title(const std::string& title)
    {
        if (state::get_sdl_state().window.sdl_window)
            SDL_SetWindowTitle(state::get_sdl_state().window.sdl_window, title.c_str());
        }

    inline std::pair<int, int> get_size() noexcept
    {
        int w = sdlcontext.width;
        int h = sdlcontext.height;

        if (sdlcontext.window)
        {
            SDL_GetWindowSize(sdlcontext.window, &w, &h);
        }

        w = (std::max)(1, w);
        h = (std::max)(1, h);
        return { w, h };
    }

    inline int sdl_get_width() noexcept { return (std::max)(1, sdlcontext.width); }
    inline int sdl_get_height() noexcept { return (std::max)(1, sdlcontext.height); }

    inline bool SDLIsRunning(std::shared_ptr<core::Context> ctx) {
        (void)ctx;
        return sdlcontext.running;
    }
#endif

}
