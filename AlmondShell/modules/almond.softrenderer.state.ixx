export module almond.softrenderer.state;

import <array>;
import <bitset>;
import <functional>;
import <memory>;
import <vector>;

import almond.core.timing;
import aengine.platform;

#if defined(ALMOND_USING_SOFTWARE_RENDERER)

export namespace almondnamespace::anativecontext
{
    using almondnamespace::timing::Timer;
    using almondnamespace::timing::createTimer;

    export struct SoftRendState
    {
#ifdef ALMOND_USING_WINMAIN
        HWND hwnd{};
        HDC  hdc{};
        HWND parent{};
        WNDPROC oldWndProc{};
        BITMAPINFO bmi{};
#endif
        std::function<void(int, int)> onResize;

        int width{ 400 };
        int height{ 300 };
        bool running{ false };

        std::vector<std::uint32_t> framebuffer;

        struct MouseState
        {
            std::array<bool, 5> down{};
            std::array<bool, 5> pressed{};
            std::array<bool, 5> prevDown{};
            int lastX{};
            int lastY{};
        } mouse;

        struct KeyboardState
        {
            std::bitset<256> down;
            std::bitset<256> pressed;
            std::bitset<256> prevDown;
        } keyboard;

        Timer pollTimer{ createTimer(1.0) };
        Timer fpsTimer{ createTimer(1.0) };
        int frameCount{};
        float angle{};
    };

    // Accessor only — no inline globals
    export SoftRendState& state();
}

// ─────────────────────────────────────────────
// MODULE IMPLEMENTATION SECTION (NO .CPP)
// ─────────────────────────────────────────────
module almond.softrenderer.state;

namespace almondnamespace::anativecontext
{
    SoftRendState& state()
    {
        static SoftRendState s{};
        return s;
    }
}

#endif // ALMOND_USING_SOFTWARE_RENDERER
