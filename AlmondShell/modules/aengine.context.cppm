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
 **************************************************************/
// aengine.context.cppm

module;

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

import aengine.context;
import aengine.input;

import "aplatform.hpp";
import "aengineconfig.hpp";
import "acontextwindow.hpp";
import "acontextmultiplexer.hpp";
import "adiagnostics.hpp";
import "aimageloader.hpp";
import "aatlastexture.hpp";
import "aatlasmanager.hpp";

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

module aengine.context;

#if defined(ALMOND_USING_OPENGL)
import :opengl;
#endif
#if defined(ALMOND_USING_SDL)
import :sdl;
#endif
#if defined(ALMOND_USING_RAYLIB)
import :raylib;
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
import :software;
#endif

namespace
{
#if defined(_WIN32)
    void ClampMouseToClientRectIfNeeded(const std::shared_ptr<almondnamespace::core::Context>& ctx, int& x, int& y) noexcept
    {
        if (!ctx || !ctx->hwnd) return;

        if (almondnamespace::input::are_mouse_coords_global()) {
            return; // Safe wrapper will translate + clamp when coordinates are global.
        }

        RECT rc{};
        if (!::GetClientRect(ctx->hwnd, &rc)) {
            return;
        }

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
        if (!ctx)
        {
            return;
        }

        if (almondnamespace::input::are_mouse_coords_global())
        {
            return;
        }

        const int width = (std::max)(1, ctx->width);
        const int height = (std::max)(1, ctx->height);

        if (x < 0 || y < 0 || x >= width || y >= height)
        {
            x = -1;
            y = -1;
        }
    }
#else
    void ClampMouseToClientRectIfNeeded(const std::shared_ptr<almondnamespace::core::Context>&, int&, int&) noexcept {}
#endif
}

namespace almondnamespace::core {

    // ─── Global backends ──────────────────────────────────────
    std::map<ContextType, BackendState> g_backends{};
    std::shared_mutex g_backendsMutex{};

    // ─── AddContextForBackend ─────────────────────────────────
    void AddContextForBackend(ContextType type, std::shared_ptr<Context> context) {
        std::unique_lock lock(g_backendsMutex);
        auto& backendState = g_backends[type];

        if (!backendState.master)
            backendState.master = std::move(context);
        else
            backendState.duplicates.emplace_back(std::move(context));

        if (!backendState.data) {
            switch (type) {
#if defined(ALMOND_USING_OPENGL)
            case ContextType::OpenGL:
                backends::opengl::ensure_backend_data(backendState);
                break;
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
            case ContextType::Software:
                backends::software::ensure_backend_data(backendState);
                break;
#endif
            default:
                break;
            }
        }
    }

    // ─── Context::process_safe ────────────────────────────────
    bool Context::process_safe(std::shared_ptr<core::Context> ctx, CommandQueue& queue) {
        if (!process) return false;
        try {
            if (ctx)
            {
                almond::diagnostics::ContextDimensionSnapshot snapshot{};
                snapshot.contextId = ctx.get();
                snapshot.type = ctx->type;
                snapshot.backendName = ctx->backendName;
                snapshot.logicalWidth = ctx->width;
                snapshot.logicalHeight = ctx->height;
                snapshot.framebufferWidth = ctx->framebufferWidth;
                snapshot.framebufferHeight = ctx->framebufferHeight;
                snapshot.virtualWidth = ctx->virtualWidth;
                snapshot.virtualHeight = ctx->virtualHeight;
                almond::diagnostics::log_context_dimensions_if_changed(snapshot);
            }
            return process(ctx, queue);
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

    inline uint32_t AddTextureThunk(TextureAtlas& atlas, std::string name, const ImageData& img, ContextType type) {
        switch (type) {
#if defined(ALMOND_USING_OPENGL)
        case ContextType::OpenGL: return backends::opengl::add_texture(atlas, std::move(name), img);
#endif
#if defined(ALMOND_USING_SDL)
        case ContextType::SDL:  return backends::sdl::add_texture(atlas, std::move(name), img);
#endif
#if defined(ALMOND_USING_SFML)
        case ContextType::SFML:  return sfmltextures::atlas_add_texture(atlas, std::move(name), img);
#endif
#if defined(ALMOND_USING_RAYLIB)
        case ContextType::RayLib: return backends::raylib::add_texture(atlas, std::move(name), img);
#endif
#if defined(ALMOND_USING_VULKAN)
        case ContextType::Vulkan: return vulkantextures::atlas_add_texture(atlas, std::move(name), img);
#endif
#if defined(ALMOND_USING_DIRECTX)
        case ContextType::DirectX: return directxtextures::atlas_add_texture(atlas, std::move(name), img);
#endif
        default: (void)atlas; (void)name; (void)img; return 0;
        }
    }

    inline uint32_t AddAtlasThunk(const TextureAtlas& atlas, ContextType type) {
        atlasmanager::ensure_uploaded(atlas);

        uint32_t handle = 0;
        switch (type) {
#if defined(ALMOND_USING_OPENGL)
        case ContextType::OpenGL: handle = backends::opengl::load_atlas(atlas); break;
#endif
#if defined(ALMOND_USING_SDL)
        case ContextType::SDL:  handle = backends::sdl::load_atlas(atlas); break;
#endif
#if defined(ALMOND_USING_SFML)
        case ContextType::SFML: handle = sfmltextures::load_atlas(atlas, atlas.get_index()); break;
#endif
#if defined(ALMOND_USING_RAYLIB)
        case ContextType::RayLib: handle = backends::raylib::load_atlas(atlas); break;
#endif
#if defined(ALMOND_USING_VULKAN)
        case ContextType::Vulkan: handle = vulkantextures::load_atlas(atlas, atlas.get_index()); break;
#endif
#if defined(ALMOND_USING_DIRECTX)
        case ContextType::DirectX: handle = directxtextures::load_atlas(atlas, atlas.get_index()); break;
#endif
        case ContextType::Software:
        case ContextType::Custom:
        case ContextType::None:
        case ContextType::Noop: {
            const int atlasIndex = atlas.get_index();
            handle = static_cast<uint32_t>(atlasIndex >= 0 ? atlasIndex + 1 : 1);
            break;
        }
        default:
            std::cerr << "[AddAtlasThunk] Unsupported context type\n";
            break;
        }
        if (handle != 0) {
            atlasmanager::process_pending_uploads(type);
        }
        return handle;
    }

    // ─── Backend stubs (minimal no-op defaults) ──────────────
#if defined(ALMOND_USING_SFML)
    inline void sfml_initialize() {}
    inline void sfml_cleanup() {}
    bool sfml_process(std::shared_ptr<core::Context> ctx, CommandQueue& queue) {
        if (!ctx) return false;
        atlasmanager::process_pending_uploads(ctx->type);
        queue.drain();
        return true;
    }
    inline void sfml_clear() {}
    inline void sfml_present() {}
    inline int  sfml_get_width() { return 800; }
    inline int  sfml_get_height() { return 600; }
#endif

#if defined(ALMOND_USING_VULKAN)
    inline void vulkan_initialize() {}
    inline void vulkan_cleanup() {}
    bool vulkan_process(std::shared_ptr<core::Context> ctx, CommandQueue& queue) {
        if (!ctx) return false;
        atlasmanager::process_pending_uploads(ctx->type);
        queue.drain();
        return true;
    }
    inline void vulkan_clear() {}
    inline void vulkan_present() {}
    inline int  vulkan_get_width() { return 800; }
    inline int  vulkan_get_height() { return 600; }
#endif

#if defined(ALMOND_USING_DIRECTX)
    inline void directx_initialize() {}
    inline void directx_cleanup() {}
    bool directx_process(std::shared_ptr<core::Context> ctx, CommandQueue& queue) {
        if (!ctx) return false;
        atlasmanager::process_pending_uploads(ctx->type);
        queue.drain();
        return true;
    }
    inline void directx_clear() {}
    inline void directx_present() {}
    inline int  directx_get_width() { return 800; }
    inline int  directx_get_height() { return 600; }
#endif

#if defined(ALMOND_USING_CUSTOM)
    inline void custom_initialize() {}
    inline void custom_cleanup() {}
    bool custom_process(std::shared_ptr<core::Context> ctx, CommandQueue& queue) {
        if (!ctx) return false;
        atlasmanager::process_pending_uploads(ctx->type);
        queue.drain();
        return true;
    }
    inline void custom_clear() {}
    inline void custom_present() {}
    inline int  custom_get_width() { return 800; }
    inline int  custom_get_height() { return 600; }
#endif

    namespace {
        inline void copy_atomic_function(AlmondAtomicFunction<uint32_t(TextureAtlas&, std::string, const ImageData&)>& dst,
            const AlmondAtomicFunction<uint32_t(TextureAtlas&, std::string, const ImageData&)>& src) {
            dst.ptr.store(src.ptr.load(std::memory_order_acquire), std::memory_order_release);
        }

        inline void copy_atomic_function(AlmondAtomicFunction<uint32_t(const TextureAtlas&)>& dst,
            const AlmondAtomicFunction<uint32_t(const TextureAtlas&)>& src) {
            dst.ptr.store(src.ptr.load(std::memory_order_acquire), std::memory_order_release);
        }

#if defined(ALMOND_USING_SFML)
        void sfml_cleanup_adapter() {
            if (auto ctx = MultiContextManager::GetCurrent()) {
                auto copy = ctx;
                almondnamespace::sfmlcontext::sfml_cleanup(copy);
            }
        }
#endif

#if defined(ALMOND_USING_RAYLIB)
        void raylib_cleanup_adapter() {
            backends::raylib::cleanup_adapter();
        }
#endif
    }

    std::shared_ptr<Context> CloneContext(const Context& prototype) {
        auto clone = std::make_shared<Context>();

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

    void InitializeAllContexts() {
        static bool s_initialized = false;
        if (s_initialized) return;
        s_initialized = true;

#if defined(ALMOND_USING_OPENGL)
        AddContextForBackend(ContextType::OpenGL, backends::opengl::make_context());
#endif

#if defined(ALMOND_USING_SDL)
        AddContextForBackend(ContextType::SDL, backends::sdl::make_context());
#endif

#if defined(ALMOND_USING_SFML)
        auto sfmlContext = std::make_shared<Context>();
        sfmlContext->initialize = sfml_initialize;
        sfmlContext->cleanup = sfml_cleanup_adapter;
        sfmlContext->process = almondnamespace::sfmlcontext::sfml_process;
        sfmlContext->clear = almondnamespace::sfmlcontext::sfml_clear;
        sfmlContext->present = almondnamespace::sfmlcontext::sfml_present;
        sfmlContext->get_width = almondnamespace::sfmlcontext::sfml_get_width;
        sfmlContext->get_height = almondnamespace::sfmlcontext::sfml_get_height;

        sfmlContext->is_key_held = [](input::Key k) { return input::is_key_held(k); };
        sfmlContext->is_key_down = [](input::Key k) { return input::is_key_down(k); };
        sfmlContext->get_mouse_position = [](int& x, int& y) {
            x = input::mouseX.load(std::memory_order_relaxed);
            y = input::mouseY.load(std::memory_order_relaxed);
        };
        sfmlContext->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
        sfmlContext->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

        sfmlContext->registry_get = [](const char*) { return 0; };
        sfmlContext->draw_sprite = sfmltextures::draw_sprite;

        sfmlContext->add_texture = [&](TextureAtlas& a, const std::string& n, const ImageData& i) {
            return AddTextureThunk(a, n, i, ContextType::SFML);
            };
        sfmlContext->add_atlas = [&](const TextureAtlas& a) {
            return AddAtlasThunk(a, ContextType::SFML);
        };
        sfmlContext->add_model = [](const char*, const char*) { return 0; };

        sfmlContext->backendName = "SFML";
        sfmlContext->type = ContextType::SFML;
        AddContextForBackend(ContextType::SFML, sfmlContext);
#endif

#if defined(ALMOND_USING_RAYLIB)
        AddContextForBackend(ContextType::RayLib, backends::raylib::make_context());
#endif

#if defined(ALMOND_USING_VULKAN)
        auto vulkanContext = std::make_shared<Context>();
        vulkanContext->initialize = vulkan_initialize;
        vulkanContext->cleanup = vulkan_cleanup;
        vulkanContext->process = vulkan_process;
        vulkanContext->clear = vulkan_clear;
        vulkanContext->present = vulkan_present;
        vulkanContext->get_width = vulkan_get_width;
        vulkanContext->get_height = vulkan_get_height;

        vulkanContext->is_key_held = [](input::Key k) { return input::is_key_held(k); };
        vulkanContext->is_key_down = [](input::Key k) { return input::is_key_down(k); };
        vulkanContext->get_mouse_position = [](int& x, int& y) {
            x = input::mouseX.load(std::memory_order_relaxed);
            y = input::mouseY.load(std::memory_order_relaxed);
        };
        vulkanContext->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
        vulkanContext->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

        vulkanContext->registry_get = [](const char*) { return 0; };
        vulkanContext->draw_sprite = [](SpriteHandle, std::span<const TextureAtlas* const>, float, float, float, float) {};

        vulkanContext->add_texture = [&](TextureAtlas& a, const std::string& n, const ImageData& i) {
            return AddTextureThunk(a, n, i, ContextType::Vulkan);
            };
        vulkanContext->add_atlas = [&](const TextureAtlas& a) {
            return AddAtlasThunk(a, ContextType::Vulkan);
        };
        vulkanContext->add_model = [](const char*, const char*) { return 0; };

        vulkanContext->backendName = "Vulkan";
        vulkanContext->type = ContextType::Vulkan;
        AddContextForBackend(ContextType::Vulkan, vulkanContext);
#endif

#if defined(ALMOND_USING_DIRECTX)
        auto directxContext = std::make_shared<Context>();
        directxContext->initialize = directx_initialize;
        directxContext->cleanup = directx_cleanup;
        directxContext->process = directx_process;
        directxContext->clear = directx_clear;
        directxContext->present = directx_present;
        directxContext->get_width = directx_get_width;
        directxContext->get_height = directx_get_height;

        directxContext->is_key_held = [](input::Key k) { return input::is_key_held(k); };
        directxContext->is_key_down = [](input::Key k) { return input::is_key_down(k); };
        directxContext->get_mouse_position = [](int& x, int& y) {
            x = input::mouseX.load(std::memory_order_relaxed);
            y = input::mouseY.load(std::memory_order_relaxed);
        };
        directxContext->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
        directxContext->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

        directxContext->registry_get = [](const char*) { return 0; };
        directxContext->draw_sprite = [](SpriteHandle, std::span<const TextureAtlas* const>, float, float, float, float) {};

        directxContext->add_texture = [&](TextureAtlas& a, const std::string& n, const ImageData& i) {
            return AddTextureThunk(a, n, i, ContextType::DirectX);
            };
        directxContext->add_atlas = [&](const TextureAtlas& a) {
            return AddAtlasThunk(a, ContextType::DirectX);
        };
        directxContext->add_model = [](const char*, const char*) { return 0; };

        directxContext->backendName = "DirectX";
        directxContext->type = ContextType::DirectX;
        AddContextForBackend(ContextType::DirectX, directxContext);
#endif

#if defined(ALMOND_USING_CUSTOM)
        auto customContext = std::make_shared<Context>();
        customContext->initialize = custom_initialize;
        customContext->cleanup = custom_cleanup;
        customContext->process = custom_process;
        customContext->clear = custom_clear;
        customContext->present = custom_present;
        customContext->get_width = custom_get_width;
        customContext->get_height = custom_get_height;

        customContext->is_key_held = [](input::Key k) { return input::is_key_held(k); };
        customContext->is_key_down = [](input::Key k) { return input::is_key_down(k); };
        customContext->get_mouse_position = [](int& x, int& y) {
            x = input::mouseX.load(std::memory_order_relaxed);
            y = input::mouseY.load(std::memory_order_relaxed);
        };
        customContext->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
        customContext->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

        customContext->registry_get = [](const char*) { return 0; };
        customContext->draw_sprite = [](SpriteHandle, std::span<const TextureAtlas* const>, float, float, float, float) {};

        customContext->add_texture = [&](TextureAtlas& a, const std::string& n, const ImageData& i) {
            return AddTextureThunk(a, n, i, ContextType::Custom);
            };
        customContext->add_atlas = [&](const TextureAtlas& a) {
            return AddAtlasThunk(a, ContextType::Custom);
        };
        customContext->add_model = [](const char*, const char*) { return 0; };

        customContext->backendName = "Custom";
        customContext->type = ContextType::Custom;
        AddContextForBackend(ContextType::Custom, customContext);
#endif

#if defined(ALMOND_USING_SOFTWARE_RENDERER)
        AddContextForBackend(ContextType::Software, backends::software::make_context());
#endif
    }

    bool ProcessAllContexts() {
        bool anyRunning = false;

        std::vector<std::shared_ptr<Context>> contexts;
        {
            std::shared_lock lock(g_backendsMutex);
            contexts.reserve(g_backends.size());
            for (auto& [_, state] : g_backends) {
                if (state.master)
                    contexts.push_back(state.master);
                for (auto& dup : state.duplicates)
                    contexts.push_back(dup);
            }
        }

        auto process_context = [&](const std::shared_ptr<Context>& ctx) {
            if (!ctx) {
                return false;
            }

            if (auto* window = ctx->windowData) {
                if (window->running) {
                    return true;
                }

                return window->commandQueue.drain();
            }

            CommandQueue localQueue;

            if (!ctx->process) {
                return localQueue.drain();
            }

            return ctx->process_safe(ctx, localQueue);
        };

        for (auto& ctx : contexts) {
            if (process_context(ctx)) {
                anyRunning = true;
            }
        }

        return anyRunning;
    }

} // namespace almondnamespace::core
