export module aengine;
export import :platform;
export import :engine_components;
export import :renderers;
export import :menu;
export import :input;
export import aallocator;
export import aatlasmanager;
export import aatlastexture;
export import aimageloader;
export import aimageatlaswriter;
export import aimagewriter;
export import atexture;
export import autilities;

#include "aengine.hpp"          // DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT

export namespace almondnamespace::core
{
    void RunEngine();
    void StartEngine();
}
