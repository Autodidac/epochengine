module;

#include <string_view>
#include <stdexcept>
#include <wtypes.h>

export module aengine.context.window;

import aframework;
import aengine.config;
import aengine;
//import aengine.platform;
//import aengine;

export namespace almondnamespace::contextwindow
{
    export struct WindowData final {
#if defined(_WIN32)
#ifndef ALMOND_MAIN_HEADLESS
        HWND hwnd = nullptr;
        HWND hwndChild = nullptr;
        HDC hdc = nullptr;
        HGLRC glrc = nullptr;
#endif
#endif

#if defined(ALMOND_USING_SDL)
        SDL_Window* sdl_window = nullptr;
        SDL_GLContext sdl_glrc = nullptr;
#endif

#if defined(ALMOND_USING_SFML)
        sf::RenderWindow* sfml_window = nullptr;
        sf::Context sfml_context;
#endif

        int width = DEFAULT_WINDOW_WIDTH;
        int height = DEFAULT_WINDOW_HEIGHT;

        bool should_close = false;

        static inline WindowData* s_instance = nullptr;

        static void set_global_instance(WindowData* instance) {
            s_instance = instance;
        }

        static WindowData* get_global_instance() noexcept {
            return s_instance;
        }

#if defined(_WIN32) && !defined(ALMOND_MAIN_HEADLESS)
        void setParentHandle(HWND hwndValue) noexcept { hwnd = hwndValue; }
        void setChildHandle(HWND hwndValue) noexcept { hwndChild = hwndValue; }

        HWND getParentHandle() const noexcept { return hwnd; }
#endif

#if defined(_WIN32)
        static HWND getWindowHandle() noexcept {
            return s_instance ? s_instance->hwnd : nullptr;
        }

        static HWND getChildHandle() noexcept {
            return s_instance ? s_instance->hwndChild : nullptr;
        }
#endif

        void set_size(int w, int h) noexcept {
            width = w;
            height = h;
        }

        void set_should_close(bool value) noexcept {
            should_close = value;
        }

        bool get_should_close() const noexcept {
            return should_close;
        }

        void set_window_title(std::string_view title) noexcept {
        }

#if defined(ALMOND_USING_SDL)
        static SDL_Window* getSDLWindow() noexcept {
            return s_instance ? s_instance->sdl_window : nullptr;
        }
#endif

#if defined(ALMOND_USING_SFML)
        sf::RenderWindow* getSFMLWindow() {
            if (!s_instance) throw std::runtime_error("WindowData is null!");
            return s_instance->sfml_window;
        }
#endif

        [[nodiscard]] int get_width() const noexcept { return width; }
        [[nodiscard]] int get_height() const noexcept { return height; }
    };
}
