module;
//#include "aplatform.hpp"
//#include "aengineconfig.hpp"
#include "aengine.config.hpp" 		// for ALMOND_USING_SDL

#if defined(ALMOND_USING_RAYLIB)
#include <raylib.h>
#endif

export module acontext.raylib.state;

import <array>;
import <bitset>;
import <functional>;
import <cstdint>;


import aengine.platform;
//import aengine.config;

import aengine.core.time;
import aengine.core.commandline;


#if defined(ALMOND_USING_RAYLIB)

namespace almondnamespace::raylibcontext
{
   struct GuiFitViewport
   {
       int vpX = 0, vpY = 0, vpW = 1, vpH = 1;
       int fbW = 1, fbH = 1;
       int refW = 1920, refH = 1080;
       float scale = 1.0f;
   };

   struct RaylibState
   {
#if defined(_WIN32)
       HWND hwnd = nullptr;
       HDC hdc = nullptr;
       HGLRC hglrc = nullptr;

       HGLRC glContext{}; // Store GL context created
       bool ownsDC{ false };
     //  WNDPROC oldWndProc = nullptr;
      // WNDPROC getOldWndProc() const noexcept { return oldWndProc; }
       HWND parent = nullptr;
#else
       void* hwnd = nullptr;
       void* hdc = nullptr;
       void* hglrc = nullptr;

       void* glContext{}; // Store GL context created
       bool ownsDC{ false };
       void* parent = nullptr;
#endif

       std::function<void(int, int)> onResize{};
       std::function<void(int, int)> clientOnResize{};
       bool dispatchingResize{ false };
       bool hasPendingResize{ false };
       unsigned int pendingWidth{ 0 };
       unsigned int pendingHeight{ 0 };
       bool pendingUpdateWindow{ false };
       bool pendingNotifyClient{ false };
       bool pendingSkipNativeApply{ false };
       unsigned int width{ 400 };
       unsigned int height{ 300 };
       unsigned int logicalWidth{ 400 };
       unsigned int logicalHeight{ 300 };
       unsigned int virtualWidth{ 400 };
       unsigned int virtualHeight{ 300 };
       unsigned int designWidth{ 0 };
       unsigned int designHeight{ 0 };
       bool running{ false };
       bool cleanupIssued{ false };

       bool shouldClose = false; // Set to true when the window should close
       // Raylib manages window internally, but track width & height for consistency
      // int screenWidth = 800;
       //int screenHeight = 600;
       int screenWidth = DEFAULT_WINDOW_WIDTH;
       int screenHeight = DEFAULT_WINDOW_HEIGHT;
       GuiFitViewport lastViewport{};
       // Mouse state (if you want to hook raw input logic)
       struct MouseState {
           std::array<bool, 5> down{};     // 5 mouse buttons
           std::array<bool, 5> pressed{};
           std::array<bool, 5> prevDown{};
           int lastX = 0, lastY = 0;
       } mouse;

       // Keyboard state (Raylib already has IsKeyDown etc — wrap if you want consistent logic)
       struct KeyboardState {
           std::bitset<512> down;      // Raylib keycodes go above 256
           std::bitset<512> pressed;
           std::bitset<512> prevDown;
       } keyboard;

       // Timing — reuse your robusttime
       timing::Timer pollTimer = timing::createTimer(1.0);
       timing::Timer fpsTimer = timing::createTimer(1.0);
       int frameCount = 0;

       // If you want shaders — Raylib supports them.
     //  Shader shader = { 0 };
       int uTransformLoc = -1;
       int uUVRegionLoc = -1;
       int uSamplerLoc = -1;

       // If you want VAOs/VBOs, stick to OpenGL path — Raylib abstracts most of that.
   };

   // Inline global instance — same pattern.
   inline RaylibState s_raylibstate{};
}

#endif // ALMON
