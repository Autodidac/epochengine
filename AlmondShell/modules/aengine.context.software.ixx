module;

import <cstdint>;
import <memory>;
import <string>;
import <utility>;

import aengine.input;
import "acontext.hpp";
import "acontextmultiplexer.hpp";
import "asoftrenderer_context.hpp";
import "asoftrenderer_renderer.hpp";
import "asoftrenderer_textures.hpp";

export module aengine.context:software;

export namespace almondnamespace::core::backends::software
{
    inline void ensure_backend_data(BackendState& backendState)
    {
        if (!backendState.data)
        {
            backendState.data = {
                new anativecontext::BackendData(),
                [](void* p) { delete static_cast<anativecontext::BackendData*>(p); }
            };
        }
    }

    inline void cleanup_adapter()
    {
        if (auto ctx = MultiContextManager::GetCurrent())
        {
            auto copy = ctx;
            almondnamespace::anativecontext::softrenderer_cleanup(copy);
        }
    }

    inline std::shared_ptr<Context> make_context()
    {
        auto softwareContext = std::make_shared<Context>();
        softwareContext->initialize = [] {};
        softwareContext->cleanup = cleanup_adapter;
        softwareContext->process = almondnamespace::anativecontext::softrenderer_process;
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
        softwareContext->add_texture = [](TextureAtlas&, const std::string&, const ImageData&) { return 0u; };
        softwareContext->add_atlas = [](const TextureAtlas& atlas) {
            const int atlasIndex = atlas.get_index();
            return static_cast<std::uint32_t>(atlasIndex >= 0 ? atlasIndex + 1 : 1);
        };
        softwareContext->add_model = [](const char*, const char*) { return 0; };

        softwareContext->backendName = "Software";
        softwareContext->type = ContextType::Software;
        return softwareContext;
    }
}
