module;

export module aengine;

export import aengine.platform;
export import aengine.engine_components;
export import aengine.renderers;
export import aengine.menu;
export import aengine.input;
export import aengine.aallocator;
export import aengine.aatlasmanager;
export import aengine.aatlastexture;
export import aengine.aimageloader;
export import aengine.aimageatlaswriter;
export import aengine.aimagewriter;
export import aengine.atexture;
export import aengine.autilities;

//#include "aengine.hpp"          // DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT

export namespace almondnamespace::core
{
    void RunEngine();
    void StartEngine();
}
