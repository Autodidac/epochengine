module;

import <cstdint>;
import <memory>;
import <string>;
import <utility>;

import aengine.input;
import "acontext.hpp";
import "acontextmultiplexer.hpp";
import "aatlasmanager.hpp";
import "araylibcontext.hpp";
import "araylibrenderer.hpp";
import "araylibtextures.hpp";

export module aengine.context:raylib;

export namespace almondnamespace::core::backends::raylib
{
    inline std::uint32_t add_texture(TextureAtlas& atlas, std::string name, const ImageData& img)
    {
        return raylibtextures::atlas_add_texture(atlas, std::move(name), img);
    }

    inline std::uint32_t load_atlas(const TextureAtlas& atlas)
    {
        return raylibtextures::load_atlas(atlas, atlas.get_index());
    }

    inline void cleanup_adapter()
    {
        if (auto ctx = MultiContextManager::GetCurrent())
        {
            auto copy = ctx;
            almondnamespace::raylibcontext::raylib_cleanup(copy);
        }
    }

    inline std::shared_ptr<Context> make_context()
    {
        auto raylibContext = std::make_shared<Context>();
        raylibContext->initialize = [] {};
        raylibContext->cleanup = cleanup_adapter;
        raylibContext->process = almondnamespace::raylibcontext::raylib_process;
        raylibContext->clear = almondnamespace::raylibcontext::raylib_clear;
        raylibContext->present = almondnamespace::raylibcontext::raylib_present;
        raylibContext->get_width = almondnamespace::raylibcontext::raylib_get_width;
        raylibContext->get_height = almondnamespace::raylibcontext::raylib_get_height;

        raylibContext->is_key_held = [](input::Key k) { return input::is_key_held(k); };
        raylibContext->is_key_down = [](input::Key k) { return input::is_key_down(k); };
        raylibContext->get_mouse_position = [](int& x, int& y) {
            x = input::mouseX.load(std::memory_order_relaxed);
            y = input::mouseY.load(std::memory_order_relaxed);
        };
        raylibContext->is_mouse_button_held = [](input::MouseButton b) { return input::is_mouse_button_held(b); };
        raylibContext->is_mouse_button_down = [](input::MouseButton b) { return input::is_mouse_button_down(b); };

        raylibContext->registry_get = [](const char*) { return 0; };
        raylibContext->draw_sprite = raylibcontext::draw_sprite;
        raylibContext->add_texture = add_texture;
        raylibContext->add_atlas = load_atlas;
        raylibContext->add_model = [](const char*, const char*) { return 0; };

        raylibContext->backendName = "RayLib";
        raylibContext->type = ContextType::RayLib;
        return raylibContext;
    }
}
