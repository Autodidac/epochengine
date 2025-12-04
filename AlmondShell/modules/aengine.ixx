export module aengine;

export import std;
export import :platform;
export import :engine_components;
export import :renderers;

#include "aengine.hpp"          // DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT

export namespace almondnamespace::core
{
    void RunEngine();
    void StartEngine();
}
