// acontext.sfml.context.ixx
module;

// -----------------------------------------------------------------------------
// Global module fragment: macros + C headers MUST live here.
// -----------------------------------------------------------------------------
#include <include/aengine.config.hpp>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  ifdef min
#    undef min
#  endif
#  ifdef max
#    undef max
#  endif
#endif

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>

export module acontext.sfml.context;

import aengine.core.context;
import aengine.context.window;
import aengine.context.commandqueue;
import aengine.core.time;
import aatlas.manager;
import acontext.sfml.state;
import acontext.sfml.textures;

import <algorithm>;
import <cmath>;
import <functional>;
import <iostream>;
import <memory>;
import <stdexcept>;
import <string>;
import <utility>;

export namespace almondnamespace::sfmlcontext
{
#if defined(ALMOND_USING_SFML)
    struct SFMLState
    {
        std::unique_ptr<sf::RenderWindow> window{};
#if defined(_WIN32)
        HWND parent = nullptr;
        HWND hwnd = nullptr;
        HDC hdc = nullptr;
        HGLRC glContext = nullptr;
#endif
        unsigned int width = 400;
        unsigned int height = 300;
        bool running = false;
        std::function<void(int, int)> onResize{};
    };

    inline SFMLState sfmlcontext{};

    inline void refresh_dimensions(const std::shared_ptr<core::Context>& ctx) noexcept
    {
        if (ctx)
        {
            ctx->width = static_cast<int>((std::max)(1u, sfmlcontext.width));
            ctx->height = static_cast<int>((std::max)(1u, sfmlcontext.height));
            ctx->virtualWidth = ctx->width;
            ctx->virtualHeight = ctx->height;
            ctx->framebufferWidth = ctx->width;
            ctx->framebufferHeight = ctx->height;
        }
    }

    inline bool sfml_initialize(std::shared_ptr<core::Context> ctx,
#if defined(_WIN32)
        HWND parentWnd = nullptr,
#else
        void* parentWnd = nullptr,
#endif
        unsigned int w = 400,
        unsigned int h = 300,
        std::function<void(int, int)> onResize = nullptr)
    {
        const unsigned int clampedWidth = (std::max)(1u, w);
        const unsigned int clampedHeight = (std::max)(1u, h);

        sfmlcontext.width = clampedWidth;
        sfmlcontext.height = clampedHeight;

#if defined(_WIN32)
        sfmlcontext.parent = parentWnd;
#else
        (void)parentWnd;
#endif

        std::weak_ptr<core::Context> weakCtx = ctx;
        auto userResize = std::move(onResize);

        sfmlcontext.onResize =
            [weakCtx, userResize = std::move(userResize)](int width, int height) mutable
            {
                sfmlcontext.width = static_cast<unsigned int>((std::max)(1, width));
                sfmlcontext.height = static_cast<unsigned int>((std::max)(1, height));

                if (sfmlcontext.window)
                    sfmlcontext.window->setSize(sf::Vector2u(sfmlcontext.width, sfmlcontext.height));

                auto locked = weakCtx.lock();
                refresh_dimensions(locked);

                state::s_sfmlstate.set_dimensions(
                    static_cast<int>(sfmlcontext.width),
                    static_cast<int>(sfmlcontext.height));

                if (userResize)
                    userResize(static_cast<int>(sfmlcontext.width), static_cast<int>(sfmlcontext.height));
            };

        if (ctx)
            ctx->onResize = sfmlcontext.onResize;

        sf::ContextSettings settings;
        settings.majorVersion = 3;
        settings.minorVersion = 3;
        settings.attributeFlags = sf::ContextSettings::Core;

        sf::VideoMode mode(sfmlcontext.width, sfmlcontext.height, 32u);
        sfmlcontext.window = std::make_unique<sf::RenderWindow>(
            mode, "SFML Window", sf::Style::Default, settings);

        if (!sfmlcontext.window || !sfmlcontext.window->isOpen())
        {
            std::cerr << "[SFML] Failed to create SFML window\n";
            return false;
        }

        auto* windowPtr = sfmlcontext.window.get();

        if (ctx && ctx->windowData)
        {
            ctx->windowData->sfml_window = windowPtr;
            ctx->windowData->set_size(static_cast<int>(sfmlcontext.width),
                static_cast<int>(sfmlcontext.height));
        }

        state::s_sfmlstate.window.sfml_window = windowPtr;

#if defined(_WIN32)
        sfmlcontext.hwnd = static_cast<HWND>(sfmlcontext.window->getSystemHandle());
        sfmlcontext.hdc = GetDC(sfmlcontext.hwnd);
        sfmlcontext.glContext = wglGetCurrentContext();
        if (!sfmlcontext.glContext)
        {
            std::cerr << "[SFML] Failed to get OpenGL context\n";
            return false;
        }

        if (sfmlcontext.parent)
        {
            SetParent(sfmlcontext.hwnd, sfmlcontext.parent);
            LONG_PTR style = GetWindowLongPtr(sfmlcontext.hwnd, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= WS_CHILD | WS_VISIBLE;
            SetWindowLongPtr(sfmlcontext.hwnd, GWL_STYLE, style);

            RECT client{};
            GetClientRect(sfmlcontext.parent, &client);
            const int width = static_cast<int>((std::max)(static_cast<LONG>(1), client.right - client.left));
            const int height = static_cast<int>((std::max)(static_cast<LONG>(1), client.bottom - client.top));

            sfmlcontext.width = static_cast<unsigned int>(width);
            sfmlcontext.height = static_cast<unsigned int>(height);

            SetWindowPos(sfmlcontext.hwnd, nullptr, 0, 0,
                width, height,
                SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            if (sfmlcontext.onResize)
                sfmlcontext.onResize(width, height);
        }
#endif

        refresh_dimensions(ctx);

        state::s_sfmlstate.set_dimensions(
            static_cast<int>(sfmlcontext.width),
            static_cast<int>(sfmlcontext.height));
        state::s_sfmlstate.running = true;

        sfmlcontext.running = true;

        atlasmanager::register_backend_uploader(core::ContextType::SFML,
            [](const TextureAtlas& atlas)
            {
                sfmlcontext::ensure_uploaded(atlas);
            });

        return true;
    }

    inline void sfml_begin_frame()
    {
        if (!sfmlcontext.window)
            throw std::runtime_error("[SFML] Window not initialized.");
        //sfmlcontext.window->clear(sf::Color::Black);
    }

    inline void sfml_end_frame()
    {
        if (!sfmlcontext.window)
            throw std::runtime_error("[SFML] Window not initialized.");
        sfmlcontext.window->display();
    }

    inline void sfml_clear() { sfml_begin_frame(); }
    inline void sfml_present() { sfml_end_frame(); }

    inline bool sfml_should_close()
    {
        return !sfmlcontext.window || !sfmlcontext.window->isOpen();
    }

    inline bool sfml_process(std::shared_ptr<core::Context> ctx, core::CommandQueue& queue)
    {
        if (!sfmlcontext.running || !sfmlcontext.window || !sfmlcontext.window->isOpen())
            return false;

        atlasmanager::process_pending_uploads(core::ContextType::SFML);

#if defined(_WIN32)
        if (!wglMakeCurrent(sfmlcontext.hdc, sfmlcontext.glContext))
        {
            std::cerr << "[SFMLRender] Failed to activate GL context\n";
            sfmlcontext.running = false;
            state::s_sfmlstate.running = false;
            return false;
        }
#endif

        if (!sfmlcontext.window->setActive(true))
        {
            std::cerr << "[SFMLRender] Failed to activate SFML window\n";
            sfmlcontext.running = false;
            state::s_sfmlstate.running = false;
#if defined(_WIN32)
            wglMakeCurrent(nullptr, nullptr);
#endif
            return false;
        }

        sf::Event event{};
        while (sfmlcontext.window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                sfmlcontext.window->close();
                sfmlcontext.running = false;
                state::s_sfmlstate.mark_should_close(true);
            }
            else if (event.type == sf::Event::Resized)
            {
                const int w = static_cast<int>((std::max)(1u, event.size.width));
                const int h = static_cast<int>((std::max)(1u, event.size.height));
                if (sfmlcontext.onResize) sfmlcontext.onResize(w, h);
            }
        }

        if (!sfmlcontext.running)
        {
            (void)sfmlcontext.window->setActive(false);
#if defined(_WIN32)
            (void)wglMakeCurrent(nullptr, nullptr);
#endif
            return false;
        }

        queue.drain();

        static auto* bgTimer = almondnamespace::timing::getTimer("menu", "bg_color");
        if (!bgTimer)
            bgTimer = &almondnamespace::timing::createNamedTimer("menu", "bg_color");

        double t = almondnamespace::timing::elapsed(*bgTimer);
        unsigned char r = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 1.0)) * 255);
        unsigned char g = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 0.7 + 2.0)) * 255);
        unsigned char b = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 1.3 + 4.0)) * 255);

        sfmlcontext.window->clear(sf::Color(r, g, b));
        sfmlcontext.window->display();

        (void)sfmlcontext.window->setActive(false);
#if defined(_WIN32)
        (void)wglMakeCurrent(nullptr, nullptr);
#endif

        return sfmlcontext.running;
    }

    inline std::pair<int, int> get_window_size_wh() noexcept
    {
        if (!sfmlcontext.window) return { 0, 0 };
        auto size = sfmlcontext.window->getSize();
        return { static_cast<int>(size.x), static_cast<int>(size.y) };
    }

    inline std::pair<int, int> get_window_position_xy() noexcept
    {
        if (!sfmlcontext.window) return { 0, 0 };
        auto pos = sfmlcontext.window->getPosition();
        return { pos.x, pos.y };
    }

    inline void set_window_position(int x, int y) noexcept
    {
        if (sfmlcontext.window)
            sfmlcontext.window->setPosition(sf::Vector2i(x, y));
    }

    inline void set_window_size(int width, int height) noexcept
    {
        if (sfmlcontext.window)
            sfmlcontext.window->setSize(sf::Vector2u(width, height));
    }

    inline void set_window_icon(const std::string& iconPath) noexcept
    {
        (void)iconPath;
        if (!sfmlcontext.window) return;
        // implement with sf::Image if desired
    }

    inline int sfml_get_width() noexcept
    {
        return sfmlcontext.window ? static_cast<int>(sfmlcontext.window->getSize().x) : 0;
    }

    inline int sfml_get_height() noexcept
    {
        return sfmlcontext.window ? static_cast<int>(sfmlcontext.window->getSize().y) : 0;
    }

    inline void sfml_set_window_title(const std::string& title) noexcept
    {
        if (sfmlcontext.window)
            sfmlcontext.window->setTitle(title);
    }

    inline void sfml_cleanup(std::shared_ptr<almondnamespace::core::Context>& ctx)
    {
        if (ctx && ctx->windowData)
            ctx->windowData->sfml_window = nullptr;

        state::s_sfmlstate.window.sfml_window = nullptr;
        state::s_sfmlstate.running = false;

        clear_gpu_atlases();

        if (sfmlcontext.running && sfmlcontext.window)
        {
            sfmlcontext.window->close();
            sfmlcontext.window.reset();
            sfmlcontext.running = false;
        }

#if defined(_WIN32)
        sfmlcontext.hwnd = nullptr;
        sfmlcontext.hdc = nullptr;
        sfmlcontext.glContext = nullptr;
        sfmlcontext.parent = nullptr;
#endif
    }

    inline bool SFMLIsRunning(std::shared_ptr<core::Context> ctx)
    {
        (void)ctx;
        return sfmlcontext.running;
    }

#endif // ALMOND_USING_SFML
} // namespace almondnamespace::sfmlcontext
