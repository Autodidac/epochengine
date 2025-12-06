module asdlstate;

#ifdef _WIN32
import <Windows.h>;
#endif

#ifdef ALMOND_USING_SDL
import <SDL3/SDL.h>;

import asdlstate;

namespace almondnamespace::sdlcontext::state
{
    SDL3State& get_sdl_state() noexcept
    {
        static SDL3State state{};
        return state;
    }
}
#endif
