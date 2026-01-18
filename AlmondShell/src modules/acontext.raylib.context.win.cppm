/**************************************************************
 *   AlmondShell – Raylib Context (Win32 glue)
 *   Purpose: Reparent raylib-created HWND into your docking host.
 *   This module may include Win32 headers. It does NOT include raylib.h.
 **************************************************************/

module;

#include <include/aengine.config.hpp>

#if defined(_WIN32) && defined(ALMOND_USING_RAYLIB)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <include/aframework.hpp>

#endif

export module acontext.raylib.context.win;

#if defined(_WIN32) && defined(ALMOND_USING_RAYLIB)

import <algorithm>;

import acontext.raylib.api;
import acontext.raylib.state;

export namespace almondnamespace::raylibcontext::win
{
    // parent is passed as void* to keep the portable layer clean.
    export inline void adopt_parent(void* parent_void) noexcept
    {
        if (!parent_void) return;

        auto& st = almondnamespace::raylibstate::s_raylibstate;

        const auto hwnd_parent = static_cast<HWND>(parent_void);

        // Cache the raylib HWND once (raylib window must already exist).
        if (!st.hwnd)
            st.hwnd = static_cast<HWND>(almondnamespace::raylib_api::get_window_handle());

        if (!st.hwnd || !hwnd_parent) return;

        st.parent = hwnd_parent;

        // Reparent into docking host.
        ::SetParent(st.hwnd, hwnd_parent);

        // Child-style conversion.
        LONG_PTR style = ::GetWindowLongPtrW(st.hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW);
        style |= (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        ::SetWindowLongPtrW(st.hwnd, GWL_STYLE, style);

        LONG_PTR ex = ::GetWindowLongPtrW(st.hwnd, GWL_EXSTYLE);
        ex &= ~static_cast<LONG_PTR>(WS_EX_APPWINDOW);
        ::SetWindowLongPtrW(st.hwnd, GWL_EXSTYLE, ex);

        // Size to parent client rect.
        RECT rc{};
        ::GetClientRect(hwnd_parent, &rc);
        const int w = (std::max)(1, int(rc.right - rc.left));
        const int h = (std::max)(1, int(rc.bottom - rc.top));

        ::SetWindowPos(
            st.hwnd, nullptr,
            0, 0, w, h,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
}

#endif
