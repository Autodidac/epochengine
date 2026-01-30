#pragma once

namespace almondnamespace::core
{
    struct BackendWindowCounts
    {
        int raylib = 1;
        int sdl = 1;
        int sfml = 1;
        int vulkan = 1;
        int opengl = 1;
        int software = 1;
    };

    BackendWindowCounts ResolveBackendWindowCounts();
}
