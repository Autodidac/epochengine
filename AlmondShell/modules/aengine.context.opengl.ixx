module;

import <cstdint>;
import <memory>;
import <string>;
import <utility>;

import aengine.input;
import "acontext.hpp";
import "acontextmultiplexer.hpp";
import "aopenglcontext.hpp";
import "aopenglrenderer.hpp";
import "aopengltextures.hpp";

export module aengine.context:opengl;

export namespace almondnamespace::core::backends::opengl
{
    inline std::uint32_t add_texture(TextureAtlas& atlas, std::string name, const ImageData& img)
    {
        return opengltextures::atlas_add_texture(atlas, std::move(name), img);
    }

    inline std::uint32_t load_atlas(const TextureAtlas& atlas)
    {
        return opengltextures::load_atlas(atlas, atlas.get_index());
    }

    inline void ensure_backend_data(BackendState& backendState)
    {
        if (!backendState.data)
        {
            backendState.data = {
                new opengltextures::BackendData(),
                [](void* p) { delete static_cast<opengltextures::BackendData*>(p); }
            };
        }
    }

    inline void clear_adapter()
    {
        if (auto ctx = MultiContextManager::GetCurrent())
        {
            almondnamespace::openglcontext::opengl_clear(ctx);
        }
    }

    inline void cleanup_adapter()
    {
        if (auto ctx = MultiContextManager::GetCurrent())
        {
            auto copy = ctx;
            almondnamespace::openglcontext::opengl_cleanup(copy);
        }
    }

    inline std::shared_ptr<Context> make_context()
    {
        auto openglContext = std::make_shared<Context>();
        openglContext->initialize = [] {};
        openglContext->cleanup = cleanup_adapter;
        openglContext->process = almondnamespace::openglcontext::opengl_process;
        openglContext->clear = clear_adapter;
        openglContext->present = almondnamespace::openglcontext::opengl_present;
        openglContext->get_width = almondnamespace::openglcontext::opengl_get_width;
        openglContext->get_height = almondnamespace::openglcontext::opengl_get_height;

        openglContext->is_key_held = [](input::Key k) { return input::is_key_held(k); };
        openglContext->is_key_down = [](input::Key k) { return input::is_key_down(k); };
        openglContext->get_mouse_position = [weakCtx = std::weak_ptr<Context>(openglContext)](int& x, int& y) {
            auto ctx = weakCtx.lock();
            if (!ctx || !ctx->windowData)
            {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
                return;
            }
            almondnamespace::core::WindowData* window = ctx->windowData;
            if (window->mouseBackend) window->mouseBackend(x, y);
            else if (window->mouseSafeBackend) window->mouseSafeBackend(x, y);
            else
            {
                x = input::mouseX.load(std::memory_order_relaxed);
                y = input::mouseY.load(std::memory_order_relaxed);
            }
        };
        openglContext->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
        openglContext->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

        openglContext->registry_get = [](const char*) { return 0; };
        openglContext->draw_sprite = almondnamespace::openglrenderer::draw_sprite;
        openglContext->add_texture = add_texture;
        openglContext->add_atlas = load_atlas;
        openglContext->add_model = almondnamespace::openglrenderer::add_model;

        openglContext->backendName = "OpenGL";
        openglContext->type = ContextType::OpenGL;
        return openglContext;
    }
}
