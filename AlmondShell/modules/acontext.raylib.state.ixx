module;

#include "aengine.config.hpp"

#if defined(ALMOND_USING_RAYLIB)
#if defined(_WIN32)
#include <windows.h>
#endif
#endif

export module acontext.raylib.state;

import <array>;
import <bitset>;
import <cstdint>;
import <functional>;

import aengine.platform;
import aengine.core.time;
import aengine.cli;

#if defined(ALMOND_USING_RAYLIB)

export namespace almondnamespace::raylibstate
{
    struct GuiFitViewport
    {
        int vpX = 0, vpY = 0, vpW = 1, vpH = 1;
        int fbW = 1, fbH = 1;
        int refW = 1920, refH = 1080;
        float scale = 1.0f;
    };

    export struct RaylibState
    {
#if defined(_WIN32)
        HWND  hwnd = nullptr;
        HDC   hdc = nullptr;
        HGLRC hglrc = nullptr;
        HWND  parent = nullptr;
#else
        void* hwnd = nullptr;
        void* hdc = nullptr;
        void* hglrc = nullptr;
        void* parent = nullptr;
#endif

        void* glContext = nullptr;
        bool  ownsDC = false;

        std::function<void(int, int)> onResize{};
        std::function<void(int, int)> clientOnResize{};

        bool dispatchingResize = false;
        bool hasPendingResize = false;
        unsigned int pendingWidth = 0;
        unsigned int pendingHeight = 0;

        bool pendingUpdateWindow = false;
        bool pendingNotifyClient = false;
        bool pendingSkipNativeApply = false;

        unsigned int width = 400;
        unsigned int height = 300;

        unsigned int logicalWidth = 400;
        unsigned int logicalHeight = 300;

        unsigned int virtualWidth = 400;
        unsigned int virtualHeight = 300;

        unsigned int designWidth = 0;
        unsigned int designHeight = 0;

        bool running = false;
        bool cleanupIssued = false;
        bool shouldClose = false;

        int screenWidth = almondnamespace::core::cli::window_width;
        int screenHeight = almondnamespace::core::cli::window_height;

        GuiFitViewport lastViewport{};

        struct MouseState
        {
            std::array<bool, 5> down{};
            std::array<bool, 5> pressed{};
            std::array<bool, 5> prevDown{};
            int lastX = 0;
            int lastY = 0;
        } mouse{};

        struct KeyboardState
        {
            std::bitset<512> down{};
            std::bitset<512> pressed{};
            std::bitset<512> prevDown{};
        } keyboard{};

        timing::Timer pollTimer = timing::createTimer(1.0);
        timing::Timer fpsTimer = timing::createTimer(1.0);
        timing::Timer frameTimer = timing::createTimer(1.0);
        int frameCount = 0;

        int uTransformLoc = -1;
        int uUVRegionLoc = -1;
        int uSamplerLoc = -1;
    };

    // One global per module instance (your existing pattern).
    inline RaylibState s_raylibstate{};

    // Exported getter used by renderer. Defined inline so it actually exists.
    export inline GuiFitViewport get_last_viewport_fit() noexcept
    {
        return s_raylibstate.lastViewport;
    }
}

#endif // ALMOND_USING_RAYLIB
