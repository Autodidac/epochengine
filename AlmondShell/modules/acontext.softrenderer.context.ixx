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
}

export namespace almondnamespace::anativecontext
{

    // Exported API (what your engine calls)
    bool softrenderer_initialize(
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

    void softrenderer_draw_quad(SoftRendState& softstate)
    {
        if (atlasmanager::atlas_vector.empty()) return;

        const auto* atlas = atlasmanager::atlas_vector.back();
        if (!atlas) return;

        const int w = atlas->width;
        const int h = atlas->height;
        if (w <= 0 || h <= 0) return;

        const std::size_t expected = std::size_t(w) * std::size_t(h) * 4u;
        if (atlas->pixel_data.size() < expected) return;

        const int dstW = (std::max)(1, softstate.width);
        const int dstH = (std::max)(1, softstate.height);

        for (int y = 0; y < softstate.height; ++y)
        {
            const int texY = std::clamp((y * h) / dstH, 0, h - 1);

            for (int x = 0; x < softstate.width; ++x)
            {
                const int texX = std::clamp((x * w) / dstW, 0, w - 1);

                const std::size_t src = (std::size_t(texY) * std::size_t(w) + std::size_t(texX)) * 4u;
                const std::uint8_t r = atlas->pixel_data[src + 0];
                const std::uint8_t g = atlas->pixel_data[src + 1];
                const std::uint8_t b = atlas->pixel_data[src + 2];
                const std::uint8_t a = atlas->pixel_data[src + 3];

                // If you want straight blit ignoring alpha, force a=255.
                const std::uint32_t packed =
                    (std::uint32_t(a) << 24) |
                    (std::uint32_t(r) << 16) |
                    (std::uint32_t(g) << 8) |
                    (std::uint32_t(b) << 0);

                softstate.framebuffer[std::size_t(y) * std::size_t(softstate.width) + std::size_t(x)] = packed;
            }
        }
    }


    inline void draw_sprite(
        SpriteHandle handle,
        std::span<const TextureAtlas* const> atlases,
        float x, float y, float width, float height) noexcept
    {
        if (!handle.is_valid())
            return;

        const int atlasIdx = static_cast<int>(handle.atlasIndex);
        const int localIdx = static_cast<int>(handle.localIndex);

        if (atlasIdx < 0 || atlasIdx >= static_cast<int>(atlases.size()))
            return;

        const TextureAtlas* atlas = atlases[atlasIdx];
        if (!atlas)
            return;

        AtlasRegion region{};
        if (!atlas->try_get_entry_info(localIdx, region))
            return;

        // Ensure pixels exist
        if (atlas->pixel_data.empty())
            const_cast<TextureAtlas*>(atlas)->rebuild_pixels();

        auto& sr = s_softrendererstate;
        if (sr.framebuffer.empty() || sr.width <= 0 || sr.height <= 0)
            return;

        // Normalize coords if caller uses 0..1
        float drawX = x;
        float drawY = y;
        float drawW = width;
        float drawH = height;

        const bool wNorm = (drawW > 0.f && drawW <= 1.f);
        const bool hNorm = (drawH > 0.f && drawH <= 1.f);

        if (wNorm)
        {
            if (drawX >= 0.f && drawX <= 1.f) drawX *= static_cast<float>(sr.width);
            drawW = (std::max)(drawW * static_cast<float>(sr.width), 1.0f);
        }
        if (hNorm)
        {
            if (drawY >= 0.f && drawY <= 1.f) drawY *= static_cast<float>(sr.height);
            drawH = (std::max)(drawH * static_cast<float>(sr.height), 1.0f);
        }

        if (drawW <= 0.f) drawW = static_cast<float>(region.width);
        if (drawH <= 0.f) drawH = static_cast<float>(region.height);

        const int destX = static_cast<int>(std::floor(drawX));
        const int destY = static_cast<int>(std::floor(drawY));
        const int destW = (std::max)(1, static_cast<int>(std::lround(drawW)));
        const int destH = (std::max)(1, static_cast<int>(std::lround(drawH)));

        const int clipX0 = (std::max)(0, destX);
        const int clipY0 = (std::max)(0, destY);
        const int clipX1 = (std::min)(sr.width, destX + destW);
        const int clipY1 = (std::min)(sr.height, destY + destH);
        if (clipX0 >= clipX1 || clipY0 >= clipY1)
            return;

        const int srcW = static_cast<int>((std::max)(1u, region.width));
        const int srcH = static_cast<int>((std::max)(1u, region.height));

        const float invDestW = 1.0f / static_cast<float>(destW);
        const float invDestH = 1.0f / static_cast<float>(destH);

        // Assumption (matches your “atlas_vector[idx] assigned” + working blit):
        // atlas->pixel_data is packed 0xAARRGGBB (u32 per pixel), same as framebuffer.
        for (int py = clipY0; py < clipY1; ++py)
        {
            const float v = (py - destY) * invDestH;
            const int sampleY = std::clamp(static_cast<int>(std::floor(v * srcH)), 0, srcH - 1);

            for (int px = clipX0; px < clipX1; ++px)
            {
                const float u = (px - destX) * invDestW;
                const int sampleX = std::clamp(static_cast<int>(std::floor(u * srcW)), 0, srcW - 1);

                const int atlasX = static_cast<int>(region.x) + sampleX;
                const int atlasY = static_cast<int>(region.y) + sampleY;

                // bounds (no signed/unsigned mismatch)
                if (static_cast<unsigned>(atlasX) >= static_cast<unsigned>(atlas->width) ||
                    static_cast<unsigned>(atlasY) >= static_cast<unsigned>(atlas->height))
                    continue;

                const size_t srcIndex =
                    static_cast<size_t>(atlasY) * static_cast<size_t>(atlas->width) + static_cast<size_t>(atlasX);

                if (srcIndex >= static_cast<size_t>(atlas->pixel_data.size()))
                    continue;

                const uint32_t src = atlas->pixel_data[srcIndex];
                const uint8_t srcA = static_cast<uint8_t>((src >> 24) & 0xFF);
                if (srcA == 0)
                    continue;

                const size_t dstIndex =
                    static_cast<size_t>(py) * static_cast<size_t>(sr.width) + static_cast<size_t>(px);

                const uint32_t dst = sr.framebuffer[dstIndex];

                const float a = static_cast<float>(srcA) / 255.0f;
                const float ia = 1.0f - a;

                const uint8_t srcR = static_cast<uint8_t>((src >> 16) & 0xFF);
                const uint8_t srcG = static_cast<uint8_t>((src >> 8) & 0xFF);
                const uint8_t srcB = static_cast<uint8_t>(src & 0xFF);

                const uint8_t dstR = static_cast<uint8_t>((dst >> 16) & 0xFF);
                const uint8_t dstG = static_cast<uint8_t>((dst >> 8) & 0xFF);
                const uint8_t dstB = static_cast<uint8_t>(dst & 0xFF);

                const uint8_t outR = static_cast<uint8_t>(srcR * a + dstR * ia + 0.5f);
                const uint8_t outG = static_cast<uint8_t>(srcG * a + dstG * ia + 0.5f);
                const uint8_t outB = static_cast<uint8_t>(srcB * a + dstB * ia + 0.5f);

                sr.framebuffer[dstIndex] =
                    (0xFFu << 24) | (uint32_t(outR) << 16) | (uint32_t(outG) << 8) | uint32_t(outB);
            }
        }
    }


    bool softrenderer_process(core::Context& ctx, core::CommandQueue& queue)
    {
        auto& sr = s_softrendererstate;

        // Clear
        std::fill(sr.framebuffer.begin(), sr.framebuffer.end(), 0xFF000000u);

        // debug fullscreen atlas blit
        // Optional draw (disabled in your header)
        //softrenderer_draw_quad(sr);

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

    void softrenderer_cleanup(std::shared_ptr<almondnamespace::core::Context>& /*ctx*/)
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

    int get_width() { return s_softrendererstate.width; }
    int get_height() { return s_softrendererstate.height; }

#else
    // If you build without ALMOND_USING_SOFTWARE_RENDERER, keep linkable stubs.
    bool softrenderer_initialize(std::shared_ptr<core::Context>, void*, unsigned, unsigned, std::function<void(int, int)>)
    {
        std::cerr << "[SoftRenderer] Not built (ALMOND_USING_SOFTWARE_RENDERER not defined)\n";
        return false;
    }
    bool softrenderer_process(core::Context&, core::CommandQueue&) { return false; }
    void softrenderer_cleanup(std::shared_ptr<almondnamespace::core::Context>&) {}
    int get_width() { return 0; }
    int get_height() { return 0; }
#endif
} // namespace almondnamespace::anativecontext
