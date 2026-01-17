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
 // aengine.context.cppm
module;

#include "aengine.config.hpp"

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#endif

export module aengine.context;

import <algorithm>;
import <cstdint>;
import <iostream>;
import <map>;
import <memory>;
import <mutex>;
import <queue>;
import <shared_mutex>;
import <span>;
import <stdexcept>;
import <string>;
import <utility>;
import <vector>;

import aengine.platform;

import aengine.input;
import aengine.core.context;
import aengine.context.window;
import aengine.context.multiplexer;

import aatlas.manager;
import aatlas.texture;
import aimage.loader;

#ifdef ALMOND_USING_VULKAN
import "avulkancontext.hpp";
import "avulkanrenderer.hpp";
import "avulkantextures.hpp";
#endif
#ifdef ALMOND_USING_DIRECTX
import "adirectxcontext.hpp";
import "adirectxrenderer.hpp";
import "adirectxtextures.hpp";
#endif
#ifdef ALMOND_USING_SFML
import "asfmlcontext.hpp";
import "asfmlrenderer.hpp";
import "asfmltextures.hpp";
#endif
#ifdef ALMOND_USING_CUSTOM
import "acustomcontext.hpp";
import "acustomrenderer.hpp";
import "acustomtextures.hpp";
#endif

#if defined(ALMOND_USING_OPENGL)
import acontext.opengl.context;
#endif
#if defined(ALMOND_USING_SDL)
import acontext.sdl.context;
import acontext.sdl.textures;
#endif
#if defined(ALMOND_USING_RAYLIB)
import acontext.raylib.context;
import acontext.raylib.renderer;
import acontext.raylib.state;
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
import acontext.softrenderer.context;
#endif

namespace
{
#if defined(_WIN32)
    void ClampMouseToClientRectIfNeeded(const std::shared_ptr<almondnamespace::core::Context>& ctx, int& x, int& y) noexcept
    {
        if (!ctx) return;

        HWND hwnd = ctx->get_hwnd();
        if (!hwnd) return;

        if (almondnamespace::input::are_mouse_coords_global())
            return;

        RECT rc{};
        if (!::GetClientRect(hwnd, &rc))
            return;

        const int width = static_cast<int>((std::max)(static_cast<LONG>(1), rc.right - rc.left));
        const int height = static_cast<int>((std::max)(static_cast<LONG>(1), rc.bottom - rc.top));

        if (x < 0 || y < 0 || x >= width || y >= height) {
            x = -1;
            y = -1;
        }
    }
#elif defined(__linux__)
    void ClampMouseToClientRectIfNeeded(const std::shared_ptr<almondnamespace::core::Context>& ctx, int& x, int& y) noexcept
    {
        if (!ctx) return;

        if (almondnamespace::input::are_mouse_coords_global())
            return;

        const int width = (std::max)(1, ctx->width);
        const int height = (std::max)(1, ctx->height);

        if (x < 0 || y < 0 || x >= width || y >= height) {
            x = -1;
            y = -1;
        }
    }
#else
    void ClampMouseToClientRectIfNeeded(const std::shared_ptr<almondnamespace::core::Context>&, int&, int&) noexcept {}
#endif

    // ------------------------------------------------------------
    // Atlas helpers that do NOT depend on removed backend APIs.
    // ------------------------------------------------------------
    std::uint32_t default_add_atlas(const almondnamespace::TextureAtlas& atlas, almondnamespace::core::ContextType type) noexcept
    {
        // Keep consistent with your old fallback behavior:
        // Use a stable non-zero handle even when no backend uploader exists.
        try {
            almondnamespace::atlasmanager::ensure_uploaded(atlas);
            almondnamespace::atlasmanager::process_pending_uploads(type);
        }
        catch (...) {
            // keep it noexcept
        }

        const int idx = atlas.get_index();
        return static_cast<std::uint32_t>(idx >= 0 ? idx + 1 : 1);
    }

    std::uint32_t default_add_texture(almondnamespace::TextureAtlas&, std::string, const almondnamespace::ImageData&) noexcept
    {
        // Old backends used to implement per-texture injection.
        // If that API is gone, this becomes a no-op placeholder.
        return 0u;
    }

    // ------------------------------------------------------------
    // Adapters: Context wants void(*)() and bool(shared_ptr, Queue&)
    // Backends often want richer signatures, so we pull Current().
    // ------------------------------------------------------------

#if defined(ALMOND_USING_OPENGL)
    void opengl_initialize_adapter()
    {
        auto ctx = almondnamespace::core::MultiContextManager::GetCurrent();
        if (!ctx) return;

        HWND hwnd = ctx->get_hwnd();
        if (!hwnd && ctx->windowData) hwnd = ctx->windowData->hwnd;

        const unsigned w = static_cast<unsigned>((std::max)(1, ctx->width));
        const unsigned h = static_cast<unsigned>((std::max)(1, ctx->height));

        try {
            (void)almondnamespace::openglcontext::opengl_initialize(ctx, hwnd, w, h, ctx->onResize);
        }
        catch (const std::exception& e) {
            std::cerr << "[OpenGL] init exception: " << e.what() << "\n";
        }
        catch (...) {
            std::cerr << "[OpenGL] init unknown exception\n";
        }
    }

    void opengl_cleanup_adapter()
    {
        auto ctx = almondnamespace::core::MultiContextManager::GetCurrent();
        if (!ctx) return;

        try {
            almondnamespace::openglcontext::opengl_cleanup(ctx);
        }
        catch (const std::exception& e) {
            std::cerr << "[OpenGL] cleanup exception: " << e.what() << "\n";
        }
        catch (...) {
            std::cerr << "[OpenGL] cleanup unknown exception\n";
        }
    }

    bool opengl_process_adapter(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        if (!ctx) return false;
        return almondnamespace::openglcontext::opengl_process(ctx, queue);
    }
#endif

#if defined(ALMOND_USING_SOFTWARE_RENDERER)
    void softrenderer_initialize_adapter()
    {
        auto ctx = almondnamespace::core::MultiContextManager::GetCurrent();
        if (!ctx) return;

        try {
            (void)almondnamespace::anativecontext::softrenderer_initialize(
                ctx,
                ctx->get_hwnd(),
                static_cast<unsigned>((std::max)(1, ctx->width)),
                static_cast<unsigned>((std::max)(1, ctx->height)),
                ctx->onResize
            );
        }
        catch (const std::exception& e) {
            std::cerr << "[SoftRenderer] init exception: " << e.what() << "\n";
        }
        catch (...) {
            std::cerr << "[SoftRenderer] init unknown exception\n";
        }
    }

    void softrenderer_cleanup_adapter()
    {
        auto ctx = almondnamespace::core::MultiContextManager::GetCurrent();
        if (!ctx) return;

        try {
            auto copy = ctx;
            almondnamespace::anativecontext::softrenderer_cleanup(copy);
        }
        catch (const std::exception& e) {
            std::cerr << "[SoftRenderer] cleanup exception: " << e.what() << "\n";
        }
        catch (...) {
            std::cerr << "[SoftRenderer] cleanup unknown exception\n";
        }
    }

    bool softrenderer_process_adapter(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        if (!ctx) return false;
        return almondnamespace::anativecontext::softrenderer_process(*ctx, queue);
    }
#endif

#if defined(ALMOND_USING_SFML)
    void sfml_cleanup_adapter()
    {
        if (auto ctx = almondnamespace::core::MultiContextManager::GetCurrent()) {
            auto copy = ctx;
            almondnamespace::sfmlcontext::sfml_cleanup(copy);
        }
    }

    bool sfml_process_adapter(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        if (!ctx) return false;
        return almondnamespace::sfmlcontext::sfml_process(*ctx, queue);
    }
#endif

#if defined(ALMOND_USING_SDL)
    void sdl_initialize_adapter()
    {
        auto ctx = almondnamespace::core::MultiContextManager::GetCurrent();
        if (!ctx) return;

        HWND parent = ctx->get_hwnd();
        if (!parent && ctx->windowData) parent = ctx->windowData->hwnd;

        try {
            (void)almondnamespace::sdlcontext::sdl_initialize(
                ctx,
                parent,
                static_cast<int>((std::max)(1, ctx->width)),
                static_cast<int>((std::max)(1, ctx->height)),
                ctx->onResize,
                ctx->backendName
            );
        }
        catch (const std::exception& e) {
            std::cerr << "[SDL] init exception: " << e.what() << "\n";
        }
        catch (...) {
            std::cerr << "[SDL] init unknown exception\n";
        }
    }

    void sdl_cleanup_adapter()
    {
        auto ctx = almondnamespace::core::MultiContextManager::GetCurrent();
        if (!ctx) return;

        try {
            auto copy = ctx;
            almondnamespace::sdlcontext::sdl_cleanup(copy);
        }
        catch (const std::exception& e) {
            std::cerr << "[SDL] cleanup exception: " << e.what() << "\n";
        }
        catch (...) {
            std::cerr << "[SDL] cleanup unknown exception\n";
        }
    }

    bool sdl_process_adapter(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        if (!ctx) return false;
        const bool running = almondnamespace::sdlcontext::sdl_process(ctx, queue);
        queue.drain();
        return running;
    }
#endif

#if defined(ALMOND_USING_RAYLIB)
    bool raylib_process_adapter(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        if (!ctx) return false;
        almondnamespace::raylibcontext::raylib_process();
        almondnamespace::atlasmanager::process_pending_uploads(almondnamespace::core::ContextType::RayLib);
        queue.drain();
        return almondnamespace::raylibstate::s_raylibstate.running;
    }
#endif
} // anonymous namespace

namespace almondnamespace::core
{
    std::map<ContextType, BackendState> g_backends{};
    std::shared_mutex g_backendsMutex{};

    void AddContextForBackend(ContextType type, std::shared_ptr<Context> context)
    {
        if (!context) return;

        std::unique_lock lock(g_backendsMutex);
        auto& backendState = g_backends[type];

        if (!backendState.master)
            backendState.master = std::move(context);
        else
            backendState.duplicates.emplace_back(std::move(context));
    }

    bool core::Context::process_safe(std::shared_ptr<core::Context> ctx, CommandQueue& queue)
    {
        if (!process) return false;
        try {
            return process(std::move(ctx), queue);
        }
        catch (const std::exception& e) {
            std::cerr << "[Context] Exception in process: " << e.what() << "\n";
            return false;
        }
        catch (...) {
            std::cerr << "[Context] Unknown exception in process\n";
            return false;
        }
    }

    namespace
    {
        inline void copy_atomic_function(
            AlmondAtomicFunction<std::uint32_t(TextureAtlas&, std::string, const ImageData&)>& dst,
            const AlmondAtomicFunction<std::uint32_t(TextureAtlas&, std::string, const ImageData&)>& src)
        {
            dst.ptr.store(src.ptr.load(std::memory_order_acquire), std::memory_order_release);
        }

        inline void copy_atomic_function(
            AlmondAtomicFunction<std::uint32_t(const TextureAtlas&)>& dst,
            const AlmondAtomicFunction<std::uint32_t(const TextureAtlas&)>& src)
        {
            dst.ptr.store(src.ptr.load(std::memory_order_acquire), std::memory_order_release);
        }

        // Default atlas hooks used when backends don't expose per-atlas upload APIs.
        std::uint32_t add_texture_default(TextureAtlas& a, std::string n, const ImageData& i)
        {
            return default_add_texture(a, std::move(n), i);
        }

        std::uint32_t add_atlas_default(const TextureAtlas& a, ContextType t)
        {
            return default_add_atlas(a, t);
        }
    }

    std::shared_ptr<core::Context> CloneContext(const core::Context& prototype)
    {
        auto clone = std::make_shared<core::Context>();

        clone->initialize = prototype.initialize;
        clone->cleanup = prototype.cleanup;
        clone->process = prototype.process;
        clone->clear = prototype.clear;
        clone->present = prototype.present;
        clone->get_width = prototype.get_width;
        clone->get_height = prototype.get_height;
        clone->registry_get = prototype.registry_get;
        clone->draw_sprite = prototype.draw_sprite;
        clone->add_model = prototype.add_model;

        clone->is_key_held = prototype.is_key_held;
        clone->is_key_down = prototype.is_key_down;
        clone->get_mouse_position = prototype.get_mouse_position;
        clone->is_mouse_button_held = prototype.is_mouse_button_held;
        clone->is_mouse_button_down = prototype.is_mouse_button_down;

        copy_atomic_function(clone->add_texture, prototype.add_texture);
        copy_atomic_function(clone->add_atlas, prototype.add_atlas);

        clone->onResize = prototype.onResize;
        clone->width = prototype.width;
        clone->height = prototype.height;
        clone->type = prototype.type;
        clone->backendName = prototype.backendName;

        clone->hwnd = nullptr;
        clone->hdc = nullptr;
        clone->hglrc = nullptr;
        clone->windowData = nullptr;

        return clone;
    }

    void InitializeAllContexts()
    {
        static bool s_initialized = false;
        if (s_initialized) return;
        s_initialized = true;

        // NOTE:
        // This file no longer calls removed backends::make_context().
        // We create prototypes by wiring known backend entrypoints
        // (or safe defaults) into core::Context.

#if defined(ALMOND_USING_OPENGL)
        {
            auto ctx = std::make_shared<Context>();
            ctx->type = ContextType::OpenGL;
            ctx->backendName = "OpenGL";

            ctx->initialize = opengl_initialize_adapter; // void(*)()
            ctx->cleanup = opengl_cleanup_adapter;    // void(*)()
            ctx->process = opengl_process_adapter;    // bool(shared_ptr, queue)
            ctx->clear = almondnamespace::openglcontext::opengl_clear;
            ctx->present = almondnamespace::openglcontext::opengl_present;
            ctx->get_width = almondnamespace::openglcontext::opengl_get_width;
            ctx->get_height = almondnamespace::openglcontext::opengl_get_height;

            ctx->is_key_held = [](input::Key k) { return input::is_key_held(k); };
            ctx->is_key_down = [](input::Key k) { return input::is_key_down(k); };
            ctx->get_mouse_position = [](int& x, int& y) {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                };
            ctx->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
            ctx->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

            ctx->add_texture = &add_texture_default;
            ctx->add_atlas = +[](const TextureAtlas& a) {
                return add_atlas_default(a, ContextType::OpenGL);
                };

            AddContextForBackend(ContextType::OpenGL, std::move(ctx));
        }
#endif

#if defined(ALMOND_USING_SDL)
        {
            auto ctx = std::make_shared<Context>();
            ctx->type = ContextType::SDL;
            ctx->backendName = "SDL";

            ctx->initialize = sdl_initialize_adapter;
            ctx->cleanup = sdl_cleanup_adapter;
            ctx->process = sdl_process_adapter;
            ctx->clear = almondnamespace::sdlcontext::sdl_clear;
            ctx->present = almondnamespace::sdlcontext::sdl_present;
            ctx->get_width = almondnamespace::sdlcontext::sdl_get_width;
            ctx->get_height = almondnamespace::sdlcontext::sdl_get_height;

            ctx->is_key_held = [](input::Key k) { return input::is_key_held(k); };
            ctx->is_key_down = [](input::Key k) { return input::is_key_down(k); };
            ctx->get_mouse_position = [](int& x, int& y) {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                };
            ctx->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
            ctx->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

            ctx->draw_sprite = sdltextures::draw_sprite;
            ctx->add_texture = &add_texture_default;
            ctx->add_atlas = +[](const TextureAtlas& a) {
                return add_atlas_default(a, ContextType::SDL);
                };

            AddContextForBackend(ContextType::SDL, std::move(ctx));
        }
#endif

#if defined(ALMOND_USING_RAYLIB)
        {
            auto ctx = std::make_shared<Context>();
            ctx->type = ContextType::RayLib;
            ctx->backendName = "RayLib";

            ctx->initialize = []() {
                auto current = almondnamespace::core::MultiContextManager::GetCurrent();
                if (!current) return;

                HWND parent = current->get_hwnd();
                if (!parent && current->windowData) parent = current->windowData->hwnd;

                try {
                    (void)almondnamespace::raylibcontext::raylib_initialize(
                        current,
                        parent,
                        static_cast<unsigned>((std::max)(1, current->width)),
                        static_cast<unsigned>((std::max)(1, current->height)),
                        current->onResize,
                        current->backendName
                    );
                }
                catch (const std::exception& e) {
                    std::cerr << "[RayLib] init exception: " << e.what() << "\n";
                }
                catch (...) {
                    std::cerr << "[RayLib] init unknown exception\n";
                }
                };
            ctx->cleanup = []() {
                auto current = almondnamespace::core::MultiContextManager::GetCurrent();
                if (!current) return;

                try {
                    almondnamespace::raylibcontext::raylib_cleanup(current);
                }
                catch (const std::exception& e) {
                    std::cerr << "[RayLib] cleanup exception: " << e.what() << "\n";
                }
                catch (...) {
                    std::cerr << "[RayLib] cleanup unknown exception\n";
                }
                };
            ctx->process = raylib_process_adapter;
            ctx->clear = []() {
                almondnamespace::raylibcontext::raylib_clear(0.0f, 0.0f, 0.0f, 1.0f);
                };
            ctx->present = almondnamespace::raylibcontext::raylib_present;
            ctx->get_width = almondnamespace::raylibcontext::raylib_get_width;
            ctx->get_height = almondnamespace::raylibcontext::raylib_get_height;

            ctx->is_key_held = [](input::Key k) { return input::is_key_held(k); };
            ctx->is_key_down = [](input::Key k) { return input::is_key_down(k); };
            ctx->get_mouse_position = [](int& x, int& y) {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                };
            ctx->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
            ctx->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

            ctx->draw_sprite = almondnamespace::raylibcontext::draw_sprite;
            ctx->add_texture = &add_texture_default;
            ctx->add_atlas = +[](const TextureAtlas& a) {
                return add_atlas_default(a, ContextType::RayLib);
                };

            AddContextForBackend(ContextType::RayLib, std::move(ctx));
        }
#endif

#if defined(ALMOND_USING_SFML)
        {
            auto ctx = std::make_shared<Context>();
            ctx->type = ContextType::SFML;
            ctx->backendName = "SFML";

            ctx->initialize = []() {};
            ctx->cleanup = sfml_cleanup_adapter;
            ctx->process = sfml_process_adapter;
            ctx->clear = []() {};
            ctx->present = []() {};
            ctx->get_width = []() { return 800; };
            ctx->get_height = []() { return 600; };

            ctx->is_key_held = [](input::Key k) { return input::is_key_held(k); };
            ctx->is_key_down = [](input::Key k) { return input::is_key_down(k); };
            ctx->get_mouse_position = [](int& x, int& y) {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                };
            ctx->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
            ctx->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

            ctx->registry_get = [](const char*) { return 0; };
            ctx->draw_sprite = sfmltextures::draw_sprite;

            ctx->add_texture = &add_texture_default;
            ctx->add_atlas = +[](const TextureAtlas& a) {
                return add_atlas_default(a, ContextType::SFML);
                };

            AddContextForBackend(ContextType::SFML, std::move(ctx));
        }
#endif

#if defined(ALMOND_USING_VULKAN)
        {
            auto ctx = std::make_shared<Context>();
            ctx->type = ContextType::Vulkan;
            ctx->backendName = "Vulkan";

            ctx->initialize = []() {};
            ctx->cleanup = []() {};
            ctx->process = [](std::shared_ptr<core::Context> c, CommandQueue& q) -> bool {
                if (!c) return false;
                atlasmanager::process_pending_uploads(c->type);
                q.drain();
                return true;
                };
            ctx->clear = []() {};
            ctx->present = []() {};
            ctx->get_width = []() { return 800; };
            ctx->get_height = []() { return 600; };

            ctx->is_key_held = [](input::Key k) { return input::is_key_held(k); };
            ctx->is_key_down = [](input::Key k) { return input::is_key_down(k); };
            ctx->get_mouse_position = [](int& x, int& y) {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                };
            ctx->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
            ctx->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

            ctx->add_texture = &add_texture_default;
            ctx->add_atlas = +[](const TextureAtlas& a) {
                return add_atlas_default(a, ContextType::Vulkan);
                };

            AddContextForBackend(ContextType::Vulkan, std::move(ctx));
        }
#endif

#if defined(ALMOND_USING_DIRECTX)
        {
            auto ctx = std::make_shared<Context>();
            ctx->type = ContextType::DirectX;
            ctx->backendName = "DirectX";

            ctx->initialize = []() {};
            ctx->cleanup = []() {};
            ctx->process = [](std::shared_ptr<core::Context> c, CommandQueue& q) -> bool {
                if (!c) return false;
                atlasmanager::process_pending_uploads(c->type);
                q.drain();
                return true;
                };
            ctx->clear = []() {};
            ctx->present = []() {};
            ctx->get_width = []() { return 800; };
            ctx->get_height = []() { return 600; };

            ctx->is_key_held = [](input::Key k) { return input::is_key_held(k); };
            ctx->is_key_down = [](input::Key k) { return input::is_key_down(k); };
            ctx->get_mouse_position = [](int& x, int& y) {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                };
            ctx->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
            ctx->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

            ctx->add_texture = &add_texture_default;
            ctx->add_atlas = +[](const TextureAtlas& a) {
                return add_atlas_default(a, ContextType::DirectX);
                };

            AddContextForBackend(ContextType::DirectX, std::move(ctx));
        }
#endif

#if defined(ALMOND_USING_CUSTOM)
        {
            auto ctx = std::make_shared<Context>();
            ctx->type = ContextType::Custom;
            ctx->backendName = "Custom";

            ctx->initialize = []() {};
            ctx->cleanup = []() {};
            ctx->process = [](std::shared_ptr<core::Context> c, CommandQueue& q) -> bool {
                if (!c) return false;
                atlasmanager::process_pending_uploads(c->type);
                q.drain();
                return true;
                };
            ctx->clear = []() {};
            ctx->present = []() {};
            ctx->get_width = []() { return 800; };
            ctx->get_height = []() { return 600; };

            ctx->is_key_held = [](input::Key k) { return input::is_key_held(k); };
            ctx->is_key_down = [](input::Key k) { return input::is_key_down(k); };
            ctx->get_mouse_position = [](int& x, int& y) {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                };
            ctx->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
            ctx->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

            ctx->add_texture = &add_texture_default;
            ctx->add_atlas = +[](const TextureAtlas& a) {
                return add_atlas_default(a, ContextType::Custom);
                };

            AddContextForBackend(ContextType::Custom, std::move(ctx));
        }
#endif

#if defined(ALMOND_USING_SOFTWARE_RENDERER)
        {
            auto ctx = std::make_shared<Context>();
            ctx->type = ContextType::Software;
            ctx->backendName = "Software";

            ctx->initialize = softrenderer_initialize_adapter;
            ctx->cleanup = softrenderer_cleanup_adapter;
            ctx->process = softrenderer_process_adapter;

            // If your software backend has these, wire them. Otherwise keep null.
            // ctx->clear = ...
            // ctx->present = ...
            // ctx->get_width = almondnamespace::anativecontext::get_width;
            // ctx->get_height = almondnamespace::anativecontext::get_height;

            ctx->is_key_held = [](input::Key k) { return input::is_key_held(k); };
            ctx->is_key_down = [](input::Key k) { return input::is_key_down(k); };
            ctx->get_mouse_position = [](int& x, int& y) {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                };
            ctx->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
            ctx->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

            ctx->add_texture = &add_texture_default;
            ctx->add_atlas = +[](const TextureAtlas& a) {
                return add_atlas_default(a, ContextType::Software);
                };

            AddContextForBackend(ContextType::Software, std::move(ctx));
        }
#endif
    }

    bool ProcessAllContexts()
    {
        bool anyRunning = false;

        std::vector<std::shared_ptr<Context>> contexts;
        {
            std::shared_lock lock(g_backendsMutex);
            contexts.reserve(g_backends.size());
            for (auto& [_, state] : g_backends) {
                if (state.master) contexts.push_back(state.master);
                for (auto& dup : state.duplicates) contexts.push_back(dup);
            }
        }

        for (auto& ctx : contexts)
        {
            if (!ctx) continue;

            if (auto* window = ctx->windowData)
            {
                if (window->running) {
                    anyRunning = true;
                }
                else {
                    window->commandQueue.drain();
                }
                continue;
            }

            CommandQueue localQueue;
            if (!ctx->process) {
                localQueue.drain();
                continue;
            }

            if (ctx->process_safe(ctx, localQueue))
                anyRunning = true;
        }

        return anyRunning;
    }
} // namespace almondnamespace::core
