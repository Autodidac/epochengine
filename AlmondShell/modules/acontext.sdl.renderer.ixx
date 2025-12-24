module;

#include "aengineconfig.hpp"

export module aengine.sdl.renderer;

export import "asdlcontextrenderer.hpp";

import std;
import <iostream>;
import <stdexcept>;
import <SDL3/SDL.h>;

import asdlstate;

#if defined(ALMOND_USING_SDL)

export namespace almondnamespace::sdlcontext
{
   // using almondnamespace::sdlcontext::state::SDL3State::s_sdlstate;

    struct RendererContext
    {
        enum class RenderMode {
            SingleTexture,
            TextureAtlas
        };
        RenderMode mode = RenderMode::TextureAtlas;

        SDL_Renderer* renderer{ nullptr };
    };

    inline RendererContext sdl_renderer{};

    inline void check_sdl_error(const char* location)
    {
        const char* err = SDL_GetError();
        if (err && *err) {
            std::cerr << "[SDL ERROR] " << location << ": " << err << "\n";
            SDL_ClearError();
        }
    }

    inline void init_renderer(SDL_Renderer* renderer)
    {
        if (!renderer) {
            throw std::runtime_error("SDL_Renderer is null");
        }
        sdl_renderer.renderer = renderer;
        SDL_SetRenderDrawColor(sdl_renderer.renderer, 0, 0, 0, 255);
        check_sdl_error("init_renderer");
    }

    inline void begin_frame()
    {
        SDL_SetRenderDrawColor(sdl_renderer.renderer, 0, 0, 0, 255);
        SDL_RenderClear(sdl_renderer.renderer);
    }

    inline void end_frame()
    {
        SDL_RenderPresent(sdl_renderer.renderer);
    }
}

#endif
