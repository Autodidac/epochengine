module;

import <cstdint>;
import <memory>;
import <string>;
import <utility>;

import aengine.input;
import "acontext.hpp";
import "acontextmultiplexer.hpp";
import "aatlasmanager.hpp";
import "asdlcontext.hpp";
import "asdlcontextrenderer.hpp";
import "asdltextures.hpp";

export module aengine.context:sdl;

export namespace almondnamespace::core::backends::sdl
{
    inline std::uint32_t add_texture(TextureAtlas& atlas, std::string name, const ImageData& img)
    {
        return sdltextures::atlas_add_texture(atlas, std::move(name), img);
    }

    inline std::uint32_t load_atlas(const TextureAtlas& atlas)
    {
        return sdltextures::load_atlas(atlas, atlas.get_index());
    }

    inline void cleanup_adapter()
    {
        if (auto ctx = MultiContextManager::GetCurrent())
        {
            auto copy = ctx;
            almondnamespace::sdlcontext::sdl_cleanup(copy);
        }
    }

    inline std::shared_ptr<Context> make_context()
    {
        auto sdlContext = std::make_shared<Context>();
        sdlContext->initialize = [] {};
        sdlContext->cleanup = cleanup_adapter;
        sdlContext->process = almondnamespace::sdlcontext::sdl_process;
        sdlContext->clear = almondnamespace::sdlcontext::sdl_clear;
        sdlContext->present = almondnamespace::sdlcontext::sdl_present;
        sdlContext->get_width = almondnamespace::sdlcontext::sdl_get_width;
        sdlContext->get_height = almondnamespace::sdlcontext::sdl_get_height;

        sdlContext->is_key_held = [](input::Key k) { return input::is_key_held(k); };
        sdlContext->is_key_down = [](input::Key k) { return input::is_key_down(k); };
        sdlContext->get_mouse_position = [](int& x, int& y) {
            x = input::mouseX.load(std::memory_order_relaxed);
            y = input::mouseY.load(std::memory_order_relaxed);
        };
        sdlContext->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
        sdlContext->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

        sdlContext->registry_get = [](const char*) { return 0; };
        sdlContext->draw_sprite = sdltextures::draw_sprite;
        sdlContext->add_texture = add_texture;
        sdlContext->add_atlas = load_atlas;
        sdlContext->add_model = [](const char*, const char*) { return 0; };

        sdlContext->backendName = "SDL";
        sdlContext->type = ContextType::SDL;
        return sdlContext;
    }
}
