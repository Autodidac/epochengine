module;

#include "aengine.hpp"          // DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT
#include "aengine.config.hpp" 		// for ALMOND_USING_SDL

#if defined(_WIN32)
#   include <windows.h>
#endif

export module aengine.context.window;

import <string>;
import <string_view>;
import <stdexcept>;
import <utility>;

import aframework;
//import aengine.config;
import aengine.context.type;          // ContextType
import aengine.context.commandqueue;  // CommandQueue
//import aengine;                       // DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT

import <functional>;
import <memory>;                      // shared_ptr

export namespace almondnamespace::core
{
    // Forward declare to avoid importing core context (keeps cycles down)
    class Context;

    export struct WindowData final
    {
        // -----------------------------------------------------------------
        // Native handles / backend-specific pointers
        // -----------------------------------------------------------------
#if defined(_WIN32)
#   if !defined(ALMOND_MAIN_HEADLESS)
        HWND  hwnd = nullptr;
        HWND  hwndChild = nullptr;
        HDC   hdc = nullptr;

        // Modern name
        HGLRC glrc = nullptr;

        // -----------------------------------------------------------------
        // Legacy compatibility aliases expected by old multiplexer code
        // -----------------------------------------------------------------
        // The multiplexer uses win->glContext; keep it as an alias.
        // NOTE: This is a *reference* to the real field (no extra storage).
        HGLRC& glContext = glrc;
#   endif
#endif

#if defined(ALMOND_USING_SDL)
        SDL_Window* sdl_window = nullptr;
        SDL_GLContext sdl_glrc = nullptr;
#endif

#if defined(ALMOND_USING_SFML)
        sf::RenderWindow* sfml_window = nullptr;
        sf::Context       sfml_context{};
#endif

        // -----------------------------------------------------------------
        // Multiplexer-owned state (render thread + command pump expects these)
        // -----------------------------------------------------------------
        std::shared_ptr<core::Context> context{};         // per-window context
        core::CommandQueue            commandQueue{};     // per-window queue

        bool running = false;
        bool usesSharedContext = false;

        core::ContextType type = core::ContextType::Custom;

        // Titles (multiplexer uses these for SetWindowText etc.)
        std::wstring titleWide{};
        std::string  titleNarrow{};

        int  width = DEFAULT_WINDOW_WIDTH;
        int  height = DEFAULT_WINDOW_HEIGHT;
        bool should_close = false;

        // -----------------------------------------------------------------
        // Legacy callback expected by multiplexer
        // -----------------------------------------------------------------
        std::function<void(int, int)> onResize{};

        // -----------------------------------------------------------------
        // Legacy helper expected by multiplexer: EnqueueCommand(...)
        // Your multiplexer calls win->EnqueueCommand(cmd).
        // We route it to CommandQueue (which already exists).
        // -----------------------------------------------------------------
        using RenderCommand = std::function<void()>;

        void EnqueueCommand(RenderCommand cmd)
        {
            // If your CommandQueue API is named differently, change ONLY here.
            commandQueue.enqueue(std::move(cmd));
        }

        // Global instance (legacy pattern)
        inline static WindowData* s_instance = nullptr;

        // Default
        WindowData() = default;

        // Win32 legacy constructor expected by multiplexer:
        // make_unique<WindowData>(HWND, HDC, HGLRC, bool, ContextType)
#if defined(_WIN32) && !defined(ALMOND_MAIN_HEADLESS)
        WindowData(HWND inHwnd, HDC inHdc, HGLRC inGlrc, bool inUsesShared, core::ContextType inType)
            : hwnd(inHwnd)
            , hdc(inHdc)
            , glrc(inGlrc)
            , usesSharedContext(inUsesShared)
            , type(inType)
        {
        }
#else
        // Still provide the signature so templates compile if it’s referenced,
        // but do nothing on non-Win/headless builds.
        WindowData(void*, void*, void*, bool inUsesShared, core::ContextType inType)
            : usesSharedContext(inUsesShared), type(inType)
        {
        }
#endif

        static void set_global_instance(WindowData* instance) noexcept { s_instance = instance; }
        static WindowData* get_global_instance() noexcept { return s_instance; }

#if defined(_WIN32) && !defined(ALMOND_MAIN_HEADLESS)
        void setParentHandle(HWND v) noexcept { hwnd = v; }
        void setChildHandle(HWND v) noexcept { hwndChild = v; }

        HWND getParentHandle() const noexcept { return hwnd; }
        HWND getChildHandle()  const noexcept { return hwndChild; }
#endif

#if defined(_WIN32)
        static HWND getWindowHandle() noexcept
        {
#   if defined(ALMOND_MAIN_HEADLESS)
            return nullptr;
#   else
            return s_instance ? s_instance->hwnd : nullptr;
#   endif
        }

        static HWND getChildHandleStatic() noexcept
        {
#   if defined(ALMOND_MAIN_HEADLESS)
            return nullptr;
#   else
            return s_instance ? s_instance->hwndChild : nullptr;
#   endif
        }
#endif

        void set_size(int w, int h) noexcept { width = w; height = h; }
        [[nodiscard]] int  get_width()  const noexcept { return width; }
        [[nodiscard]] int  get_height() const noexcept { return height; }

        void set_should_close(bool v) noexcept { should_close = v; }
        [[nodiscard]] bool get_should_close() const noexcept { return should_close; }

        void set_window_title(std::string_view title) noexcept
        {
            titleNarrow.assign(title.begin(), title.end());
#if defined(_WIN32)
            titleWide.assign(titleNarrow.begin(), titleNarrow.end());
#endif
        }

#if defined(ALMOND_USING_SDL)
        static SDL_Window* getSDLWindow() noexcept { return s_instance ? s_instance->sdl_window : nullptr; }
#endif

#if defined(ALMOND_USING_SFML)
        sf::RenderWindow* getSFMLWindow()
        {
            if (!s_instance) throw std::runtime_error("WindowData is null!");
            return s_instance->sfml_window;
        }
#endif
    };
}

// Compatibility: keep old namespace working
export namespace almondnamespace::contextwindow
{
    export using WindowData = almondnamespace::core::WindowData;
}
