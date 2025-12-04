module;

export module aengine:renderers;

import std;

#ifdef ALMOND_USING_SDL
import "asdlcontext.hpp";
#endif
#ifdef ALMOND_USING_SFML
import "asfmlcontext.hpp";
#endif
#ifdef ALMOND_USING_SOFTWARE_RENDERER
import "asoftrenderer_context.hpp";
#endif
#ifdef ALMOND_USING_RAYLIB
import "araylibcontext.hpp";
#endif

export namespace almondnamespace
{
#ifdef ALMOND_USING_SDL
    namespace sdl
    {
        using namespace ::almondnamespace::sdl;
    }
#endif

#ifdef ALMOND_USING_SFML
    namespace sfml
    {
        using namespace ::almondnamespace::sfml;
    }
#endif

#ifdef ALMOND_USING_SOFTWARE_RENDERER
    namespace softrenderer
    {
        using namespace ::almondnamespace::softrenderer;
    }
#endif

#ifdef ALMOND_USING_RAYLIB
    namespace raylib
    {
        using namespace ::almondnamespace::raylib;
    }
#endif
}
