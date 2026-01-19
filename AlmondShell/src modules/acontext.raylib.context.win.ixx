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

// IMPORTANT: we include Win32, but we do NOT include raylib.h here.
#include <include/aframework.hpp>
#include <wingdi.h> // HDC/HGLRC + wgl*

#endif

export module acontext.raylib.context.win;

#if defined(_WIN32) && defined(ALMOND_USING_RAYLIB)

import <algorithm>;

import acontext.raylib.api;
import acontext.raylib.state;

namespace almondnamespace::raylibcontext::win::detail
{
    [[nodiscard]] inline HGLRC current_context() noexcept { return ::wglGetCurrentContext(); }
    [[nodiscard]] inline HDC   current_dc() noexcept { return ::wglGetCurrentDC(); }

    inline bool make_current(HDC dc, HGLRC rc) noexcept
    {
        return ::wglMakeCurrent(dc, rc) != FALSE;
    }

    inline void clear_current() noexcept
    {
        (void)::wglMakeCurrent(nullptr, nullptr);
    }
}

export namespace almondnamespace::raylibcontext::win
{
    // parent is passed as void* to keep the portable layer clean.
    export inline void adopt_parent(void* parent_void) noexcept
    {
        if (!parent_void) return;

        auto& st = almondnamespace::raylibstate::s_raylibstate;
        const auto hwnd_parent = static_cast<HWND>(parent_void);
        if (!hwnd_parent) return;

        // Cache raylib HWND once (raylib window must already exist).
        if (!st.hwnd)
            st.hwnd = static_cast<HWND>(almondnamespace::raylib_api::get_window_handle());

        if (!st.hwnd) return;

        // If already adopted into same parent, just resize.
        const HWND current_parent = ::GetParent(st.hwnd);
        st.parent = hwnd_parent;

        // Save previous GL binding (for the multiplexer/docking host).
        const HDC   prev_dc = detail::current_dc();
        const HGLRC prev_rc = detail::current_context();

        // Reparent into docking host (only if needed).
        if (current_parent != hwnd_parent)
            ::SetParent(st.hwnd, hwnd_parent);

        // Make it a real child window.
        LONG_PTR style = ::GetWindowLongPtrW(st.hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW);
        style &= ~static_cast<LONG_PTR>(WS_POPUP);
        style |= (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        ::SetWindowLongPtrW(st.hwnd, GWL_STYLE, style);

        LONG_PTR ex = ::GetWindowLongPtrW(st.hwnd, GWL_EXSTYLE);
        ex &= ~static_cast<LONG_PTR>(WS_EX_APPWINDOW);
        ex &= ~static_cast<LONG_PTR>(WS_EX_TOOLWINDOW);
        ::SetWindowLongPtrW(st.hwnd, GWL_EXSTYLE, ex);

        // Prevent weird ownership/activation behavior.
        ::SetWindowLongPtrW(st.hwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hwnd_parent));

        // Size to parent client rect.
        RECT rc{};
        ::GetClientRect(hwnd_parent, &rc);
        const int w = (std::max)(1, int(rc.right - rc.left));
        const int h = (std::max)(1, int(rc.bottom - rc.top));

        ::SetWindowPos(
            st.hwnd, nullptr,
            0, 0, w, h,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        // Force a resize path (some GLFW builds won't update framebuffer without messages).
        ::SendMessageW(st.hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(w, h));

        // --- CRITICAL PART ---
        // After reparent/style changes, reacquire the DC and current RC that raylib/GLFW is using.
        // Then store them into state so your make_current_guard works reliably.
        //
        // We deliberately do NOT "own" the DC (GLFW/raylib manages it); we only cache it.
        st.ownsDC = false;

        // Prefer raylib's current binding *if it is current right now*.
        // If it's not current, fall back to GetDC + wglGetCurrentContext if possible.
        HDC   ray_dc = detail::current_dc();
        HGLRC ray_rc = detail::current_context();

        if (!ray_dc)
            ray_dc = ::GetDC(st.hwnd); // temporary grab; we won't ReleaseDC because we don't own it
        if (!ray_rc)
            ray_rc = detail::current_context();

        st.hdc = ray_dc;
        st.hglrc = ray_rc;

        // Restore previous GL binding for the dock/multiplexer host.
        if (prev_dc && prev_rc)
            (void)detail::make_current(prev_dc, prev_rc);
        else
            detail::clear_current();
    }
}

#endif
