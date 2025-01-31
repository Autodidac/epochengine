// awindow.h
#pragma once

#include <functional>
#include <stdexcept>

#ifdef _WIN32
#include "aframework.h"
#endif

namespace almondnamespace::contextwindow {

    // Cross-platform window handle type
#ifdef _WIN32
    using WindowHandle = HWND;
#else
    using WindowHandle = void*; // Fallback for non-Windows platforms
#endif

    // Base WindowContext utility
    struct WindowContext {
        static WindowHandle window_handle; // Static variable, internal linkage

        // Get the current window handle
        static inline WindowHandle getWindowHandle() {
            return window_handle;
        }

        // Set the current window handle
        static inline void setWindowHandle(WindowHandle handle) {
            window_handle = handle;
        }
    };

    // Initialize the static window handle
    inline WindowHandle WindowContext::window_handle = nullptr;

} // namespace almondshell::contextwindow
