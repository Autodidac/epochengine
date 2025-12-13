module;

export module aengine:renderers;

#ifdef ALMOND_USING_SDL
import asdlcontext;
import asdlcontextrenderer;
import asdltextures;
#endif
#ifdef ALMOND_USING_SFML
import asfmlcontext;
import asfmlrenderer;
import asfmltextures;
#endif
#ifdef ALMOND_USING_SOFTWARE_RENDERER
import asoftrenderer_context;
import asoftrenderer_renderer;
import asoftrenderer_textures;
import asoftrenderer_quad;
import asoftrenderer_state;
#endif
#ifdef ALMOND_USING_RAYLIB
import araylibcontext;
import araylibcontextinput;
import araylibrenderer;
import araylibtextures;
import araylibstate;
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
