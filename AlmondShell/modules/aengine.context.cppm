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
import aimage.loader;
import aatlas.texture;
import aatlas.manager;

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
#endif
#if defined(ALMOND_USING_RAYLIB)
import acontext.raylib.context;
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

        if (almondnamespace::input::are_mouse_coords_global()) {
            return;
        }

        RECT rc{};
        if (!::GetClientRect(hwnd, &rc)) {
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
        if (!ctx) return;

        if (almondnamespace::input::are_mouse_coords_global()) {
            return;
        }

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

    // ---------------------------------------------------------------------
    // Adapters: Context wants void(*)() and bool(shared_ptr<Context>,Queue&)
    // Many backends use richer signatures. Pull current ctx at call time.
    // ---------------------------------------------------------------------

#if defined(ALMOND_USING_SOFTWARE_RENDERER)
    void softrenderer_initialize_adapter()
    {
        auto ctx = almondnamespace::core::MultiContextManager::GetCurrent();
        if (!ctx) return;

        // Expected (based on your prior compile error pattern):
        // bool softrenderer_initialize(std::shared_ptr<Context>, HWND, u32, u32, std::function<void(int,int)>)
        (void)almondnamespace::anativecontext::softrenderer_initialize(
            ctx,
            ctx->get_hwnd(),
            static_cast<unsigned>(ctx->width),
            static_cast<unsigned>(ctx->height),
            ctx->onResize
        );
    }

    void softrenderer_cleanup_adapter()
    {
        auto ctx = almondnamespace::core::MultiContextManager::GetCurrent();
        if (!ctx) return;

        // Expected (based on your prior compile error pattern):
        // void softrenderer_cleanup(std::shared_ptr<Context>&)
        auto copy = ctx;
        almondnamespace::anativecontext::softrenderer_cleanup(copy);
    }

    bool softrenderer_process_adapter(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        if (!ctx) return false;
        // Expected mismatch you hit earlier: bool process(Context&, Queue&)
        return almondnamespace::anativecontext::softrenderer_process(*ctx, queue);
    }
#endif

#if defined(ALMOND_USING_SFML)
    bool sfml_process_adapter(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        if (!ctx) return false;

        // If your SFML process already matches the shared_ptr signature, this still works
        // only if there is an overload taking Context&; otherwise switch to direct call.
        return almondnamespace::sfmlcontext::sfml_process(*ctx, queue);
    }
#endif
}

namespace almondnamespace::core
{
    // ─── Global backends ──────────────────────────────────────
    std::map<ContextType, BackendState> g_backends{};
    std::shared_mutex g_backendsMutex{};

    // ─── AddContextForBackend ─────────────────────────────────
    void AddContextForBackend(ContextType type, std::shared_ptr<almondnamespace::core::Context> context)
    {
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
            default:
                break;
            }
        }
    }

    // ─── Context::process_safe ────────────────────────────────
    bool core::Context::process_safe(std::shared_ptr<core::Context> ctx, CommandQueue& queue)
    {
        if (!process) return false;
        try {
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

    inline std::uint32_t AddTextureThunk(TextureAtlas& atlas, std::string name, const ImageData& img, ContextType type)
    {
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
        default: (void)atlas; (void)name; (void)img; return 0u;
        }
    }

    inline std::uint32_t AddAtlasThunk(const TextureAtlas& atlas, ContextType type)
    {
        atlasmanager::ensure_uploaded(atlas);

        std::uint32_t handle = 0;
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
            handle = static_cast<std::uint32_t>(atlasIndex >= 0 ? atlasIndex + 1 : 1);
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

#if defined(ALMOND_USING_SFML)
        void sfml_cleanup_adapter()
        {
            if (auto ctx = MultiContextManager::GetCurrent()) {
                auto copy = ctx;
                almondnamespace::sfmlcontext::sfml_cleanup(copy);
            }
        }
#endif

#if defined(ALMOND_USING_RAYLIB)
        void raylib_cleanup_adapter()
        {
            backends::raylib::cleanup_adapter();
        }
#endif
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

#if defined(ALMOND_USING_OPENGL)
        AddContextForBackend(ContextType::OpenGL, backends::opengl::make_context());
#endif

#if defined(ALMOND_USING_SDL)
        AddContextForBackend(ContextType::SDL, backends::sdl::make_context());
#endif

#if defined(ALMOND_USING_SFML)
        auto sfmlContext = std::make_shared<Context>();
        sfmlContext->initialize = []() {};                 // must be void(*)()
        sfmlContext->cleanup = sfml_cleanup_adapter;       // must be void(*)()
        sfmlContext->process = sfml_process_adapter;       // force correct signature
        sfmlContext->clear = []() {};
        sfmlContext->present = []() {};
        sfmlContext->get_width = []() { return 800; };
        sfmlContext->get_height = []() { return 600; };

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
        vulkanContext->initialize = []() {};
        vulkanContext->cleanup = []() {};
        vulkanContext->process = [](std::shared_ptr<core::Context> ctx, CommandQueue& queue) -> bool {
            if (!ctx) return false;
            atlasmanager::process_pending_uploads(ctx->type);
            queue.drain();
            return true;
            };
        vulkanContext->clear = []() {};
        vulkanContext->present = []() {};
        vulkanContext->get_width = []() { return 800; };
        vulkanContext->get_height = []() { return 600; };

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
        directxContext->initialize = []() {};
        directxContext->cleanup = []() {};
        directxContext->process = [](std::shared_ptr<core::Context> ctx, CommandQueue& queue) -> bool {
            if (!ctx) return false;
            atlasmanager::process_pending_uploads(ctx->type);
            queue.drain();
            return true;
            };
        directxContext->clear = []() {};
        directxContext->present = []() {};
        directxContext->get_width = []() { return 800; };
        directxContext->get_height = []() { return 600; };

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
        customContext->initialize = []() {};
        customContext->cleanup = []() {};
        customContext->process = [](std::shared_ptr<core::Context> ctx, CommandQueue& queue) -> bool {
            if (!ctx) return false;
            atlasmanager::process_pending_uploads(ctx->type);
            queue.drain();
            return true;
            };
        customContext->clear = []() {};
        customContext->present = []() {};
        customContext->get_width = []() { return 800; };
        customContext->get_height = []() { return 600; };

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
        auto softwareContext = std::make_shared<Context>();

        // FIXED: these must match Context's raw hook types.
        softwareContext->initialize = softrenderer_initialize_adapter;      // void(*)()
        softwareContext->cleanup = softrenderer_cleanup_adapter;            // void(*)()
        softwareContext->process = softrenderer_process_adapter;            // bool(shared_ptr, queue)

        softwareContext->clear = nullptr;
        softwareContext->present = nullptr;
        softwareContext->get_width = almondnamespace::anativecontext::get_width;
        softwareContext->get_height = almondnamespace::anativecontext::get_height;

        softwareContext->is_key_held = [](input::Key k) { return input::is_key_held(k); };
        softwareContext->is_key_down = [](input::Key k) { return input::is_key_down(k); };
        softwareContext->get_mouse_position = [](int& x, int& y) {
            x = input::mouseX.load(std::memory_order_relaxed);
            y = input::mouseY.load(std::memory_order_relaxed);
            };
        softwareContext->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
        softwareContext->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

        softwareContext->registry_get = [](const char*) { return 0; };
        softwareContext->draw_sprite = anativecontext::draw_sprite;

        softwareContext->add_texture = [&](TextureAtlas& a, const std::string& n, const ImageData& i) {
            return AddTextureThunk(a, n, i, ContextType::Software);
            };
        softwareContext->add_atlas = [&](const TextureAtlas& a) {
            return AddAtlasThunk(a, ContextType::Software);
            };
        softwareContext->add_model = [](const char*, const char*) { return 0; };

        softwareContext->backendName = "Software";
        softwareContext->type = ContextType::Software;
        AddContextForBackend(ContextType::Software, softwareContext);
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

            if (auto* window = ctx->windowData) {
                // render thread drives it; just report "still alive" if window says running
                if (window->running) {
                    anyRunning = true;
                }
                else {
                    // thread stopped; drain any leftover commands
                    window->commandQueue.drain();
                }
                continue;
            }

            CommandQueue localQueue;
            if (!ctx->process) {
                localQueue.drain();
                continue;
            }

            if (ctx->process_safe(ctx, localQueue)) {
                anyRunning = true;
            }
        }

        return anyRunning;
    }

} // namespace almondnamespace::core
