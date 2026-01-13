/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 *                                                            *
 *   This file is part of the Almond Project.                 *
 *   AlmondEngine - Modular C++ Game Engine                   *
 *                                                            *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
 *                                                            *
 *   Provided "AS IS", without warranty of any kind.          *
 *   Use permitted for non-commercial purposes only           *
 *   without prior commercial licensing agreement.            *
 *                                                            *
 *   Redistribution allowed with this notice.                 *
 *   No obligation to disclose modifications.                 *
 *   See LICENSE file for full terms.                         *
 **************************************************************/
 //
 // acontext.softrenderer.context.ixx
 //
 // Key fixes vs your header / the broken module you had:
 //  - NO direct ctx->hwnd / ctx->hdc member access (your Context has those private now).
 //  - Backend does not assume it owns/destroys any HWND.
 //  - Uses C++23 requires-expressions to *optionally* grab HWND/HDC via accessors if they exist.
 //    Otherwise, you must pass parentWnd explicitly from the multiplexer (recommended).
 //

module;

//#include "aplatform.hpp"
#include "aengine.config.hpp"

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#endif

export module acontext.softrenderer.context;

import aengine.platform;

import <algorithm>;
import <chrono>;
import <cstdint>;
import <functional>;
import <iostream>;
import <memory>;
import <utility>;
import <vector>;

import aengine.core.context;             // almondnamespace::core::Context
import aengine.context.commandqueue;     // almondnamespace::core::CommandQueue

import acontext.softrenderer.state;      // s_softrendererstate, SoftRendState
import acontext.softrenderer.textures;   // Texture, TexturePtr (as in your project)
import acontext.softrenderer.renderer;   // SoftwareRenderer (as in your project)
import aatlas.manager;                  // atlasmanager::atlas_vector (as in your header)

namespace almondnamespace::anativecontext
{
#if defined(ALMOND_USING_SOFTWARE_RENDERER)

    // These stay module-internal; nobody else should poke them directly.
    inline TexturePtr       cubeTexture{};
    inline SoftwareRenderer renderer{};

#if defined(_WIN32)
    // --- Optional accessors for HWND/HDC without assuming member names exist ---
    template <class T>
    static HWND try_get_hwnd(const T& ctx)
    {
        if constexpr (requires { ctx.get_hwnd(); })
            return reinterpret_cast<HWND>(ctx.get_hwnd());
        else if constexpr (requires { ctx.hwnd(); })
            return reinterpret_cast<HWND>(ctx.hwnd());
        else if constexpr (requires { ctx.native_hwnd(); })
            return reinterpret_cast<HWND>(ctx.native_hwnd());
        else if constexpr (requires { ctx.native_window_handle(); })
            return reinterpret_cast<HWND>(ctx.native_window_handle());
        else
            return nullptr;
    }

    template <class T>
    static HDC try_get_hdc(const T& ctx)
    {
        if constexpr (requires { ctx.get_hdc(); })
            return reinterpret_cast<HDC>(ctx.get_hdc());
        else if constexpr (requires { ctx.hdc(); })
            return reinterpret_cast<HDC>(ctx.hdc());
        else if constexpr (requires { ctx.native_hdc(); })
            return reinterpret_cast<HDC>(ctx.native_hdc());
        else
            return nullptr;
    }
#endif

    // Exported API (what your engine calls)
    export bool softrenderer_initialize(
        std::shared_ptr<core::Context> ctx,
#if defined(_WIN32)
        HWND parentWnd = nullptr,
#else
        void* parentWnd = nullptr,
#endif
        unsigned int w = 400,
        unsigned int h = 300,
        std::function<void(int, int)> onResize = nullptr)
    {
        if (!ctx)
        {
            std::cerr << "[SoftRenderer] Invalid context\n";
            return false;
        }

        auto& sr = s_softrendererstate;
        sr.width = static_cast<int>(w);
        sr.height = static_cast<int>(h);
        sr.running = true;
        sr.onResize = std::move(onResize);

        // Allocate framebuffer
        sr.framebuffer.assign(std::size_t(w) * std::size_t(h), 0xFF000000u);

#if defined(_WIN32)
        // Prefer explicit parentWnd from multiplexer; fall back to accessor if it exists.
        HWND resolvedParent = parentWnd;
        if (!resolvedParent)
            resolvedParent = try_get_hwnd(*ctx);

        if (!resolvedParent)
        {
            std::cerr << "[SoftRenderer] No parent HWND available. Pass parentWnd from multiplexer.\n";
            return false;
        }

        sr.parent = resolvedParent;
        sr.hwnd = resolvedParent; // store for presentation target only (NOT owned)

        // Prebuild BITMAPINFO for StretchDIBits
        ZeroMemory(&sr.bmi, sizeof(BITMAPINFO));
        sr.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        sr.bmi.bmiHeader.biWidth = sr.width;
        sr.bmi.bmiHeader.biHeight = -sr.height; // top-down
        sr.bmi.bmiHeader.biPlanes = 1;
        sr.bmi.bmiHeader.biBitCount = 32;
        sr.bmi.bmiHeader.biCompression = BI_RGB;

        std::cout << "[SoftRenderer] Initialized. HWND=" << sr.hwnd
            << " (" << sr.width << "x" << sr.height << ")\n";
#else
        (void)parentWnd;
        std::cout << "[SoftRenderer] Initialized (non-Win32) "
            << sr.width << "x" << sr.height << "\n";
#endif

        // Demo texture (kept from your header)
        if (!cubeTexture)
        {
            cubeTexture = std::make_shared<Texture>(64, 64);
            for (int y = 0; y < cubeTexture->height; ++y)
            {
                for (int x = 0; x < cubeTexture->width; ++x)
                {
                    cubeTexture->pixels[std::size_t(y) * std::size_t(cubeTexture->width) + std::size_t(x)] =
                        ((x / 8 + y / 8) % 2) ? 0xFFFF0000u : 0xFF00FF00u;
                }
            }
        }

        return true;
    }

    static void softrenderer_draw_quad(SoftRendState& softstate)
    {
        if (atlasmanager::atlas_vector.empty()) return;

        const auto* atlas = atlasmanager::atlas_vector[0];
        if (!atlas) return;

        const int w = atlas->width;
        const int h = atlas->height;
        if (w <= 0 || h <= 0 || atlas->pixel_data.empty()) return;

        for (int y = 0; y < softstate.height; ++y)
        {
            for (int x = 0; x < softstate.width; ++x)
            {
                const float u = float(x) / float((std::max)(1, softstate.width));
                const float v = float(y) / float((std::max)(1, softstate.height));

                const int texX = static_cast<int>(u * float(w - 1));
                const int texY = static_cast<int>(v * float(h - 1));

                const std::size_t idx = std::size_t(texY) * std::size_t(w) + std::size_t(texX);
                if (idx < atlas->pixel_data.size())
                    softstate.framebuffer[std::size_t(y) * std::size_t(softstate.width) + std::size_t(x)] =
                    atlas->pixel_data[idx];
            }
        }
    }

    export bool softrenderer_process(core::Context& ctx, core::CommandQueue& queue)
    {
        auto& sr = s_softrendererstate;

        // Clear
        std::fill(sr.framebuffer.begin(), sr.framebuffer.end(), 0xFF000000u);

        // Optional draw (disabled in your header)
        // softrenderer_draw_quad(sr);

        // Drain commands
        queue.drain();

#if defined(_WIN32)
        // Present
        // Prefer HDC accessor if it exists; otherwise use GetDC on the stored HWND.
        HDC hdc = try_get_hdc(ctx);
        bool tempDC = false;

        if (!hdc && sr.hwnd)
        {
            hdc = GetDC(sr.hwnd);
            tempDC = (hdc != nullptr);
        }

        if (hdc)
        {
            StretchDIBits(
                hdc,
                0, 0, sr.width, sr.height,
                0, 0, sr.width, sr.height,
                sr.framebuffer.data(),
                &sr.bmi,
                DIB_RGB_COLORS,
                SRCCOPY);

            if (tempDC && sr.hwnd)
                ReleaseDC(sr.hwnd, hdc);
        }
#endif
        return true;
    }

    export void softrenderer_cleanup(std::shared_ptr<almondnamespace::core::Context>& /*ctx*/)
    {
        auto& sr = s_softrendererstate;

        sr.framebuffer.clear();
        cubeTexture.reset();

        // DO NOT DestroyWindow here. This backend does not own the window.
        sr.hwnd = nullptr;
        sr.parent = nullptr;
        sr.running = false;

        sr = {}; // reset remaining fields

        std::cout << "[SoftRenderer] Cleanup complete\n";
    }

    export int get_width() { return s_softrendererstate.width; }
    export int get_height() { return s_softrendererstate.height; }

#else
    // If you build without ALMOND_USING_SOFTWARE_RENDERER, keep linkable stubs.
    export bool softrenderer_initialize(std::shared_ptr<core::Context>, void*, unsigned, unsigned, std::function<void(int, int)>)
    {
        std::cerr << "[SoftRenderer] Not built (ALMOND_USING_SOFTWARE_RENDERER not defined)\n";
        return false;
    }
    export bool softrenderer_process(core::Context&, core::CommandQueue&) { return false; }
    export void softrenderer_cleanup(std::shared_ptr<almondnamespace::core::Context>&) {}
    export int get_width() { return 0; }
    export int get_height() { return 0; }
#endif
} // namespace almondnamespace::anativecontext
