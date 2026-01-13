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
 // acontext.softrenderer.quad.ixx

module;

#include <algorithm>
#include <cstdint>

export module acontext.softrenderer.quad;

import acontext.softrenderer.textures; // BackendData, Texture, TexturePtr, create_texture
import aatlas.manager;                 // atlasmanager::atlas_vector (and atlas types)

export namespace almondnamespace::anativecontext
{
#if defined(ALMOND_USING_SOFTWARE_RENDERER)

    // ─── Draw a textured quad into the software framebuffer ────────────────────
    inline void draw_textured_quad(
        BackendData& backend,
        const Texture& tex,
        int dstX, int dstY, int dstW, int dstH)
    {
        if (backend.srState.pixels.empty()) return;

        const int fbW = backend.srState.width;
        const int fbH = backend.srState.height;

        for (int y = 0; y < dstH; ++y)
        {
            const int fbY = dstY + y;
            if (fbY < 0 || fbY >= fbH) continue;

            const float v = static_cast<float>(y) / static_cast<float>(dstH);
            const int srcY = static_cast<int>(v * static_cast<float>(tex.height));

            for (int x = 0; x < dstW; ++x)
            {
                const int fbX = dstX + x;
                if (fbX < 0 || fbX >= fbW) continue;

                const float u = static_cast<float>(x) / static_cast<float>(dstW);
                const int srcX = static_cast<int>(u * static_cast<float>(tex.width));

                const std::uint32_t src = tex.sample(srcX, srcY);
                backend.srState.pixels[fbY * fbW + fbX] = src;
            }
        }
    }

    // ─── High-level entry: blit first atlas onto framebuffer ───────────────────
    inline void render_first_atlas_quad(BackendData& backend)
    {
        if (atlasmanager::atlas_vector.empty()) return;

        const auto* atlas = atlasmanager::atlas_vector[0];
        if (!atlas) return;

        // Upload atlas → TexturePtr if not already cached
        auto it = backend.textures.find(atlas);
        if (it == backend.textures.end())
        {
            auto tex = create_texture(atlas->width, atlas->height);

            std::copy(atlas->pixel_data.begin(),
                atlas->pixel_data.end(),
                tex->pixels.begin());

            backend.textures[atlas] = tex;
            it = backend.textures.find(atlas);
        }

        TexturePtr tex = it->second;
        if (!tex) return;

        // Fullscreen quad
        draw_textured_quad(backend, *tex, 0, 0,
            backend.srState.width,
            backend.srState.height);
    }

#endif // ALMOND_USING_SOFTWARE_RENDERER
} // namespace almondnamespace::anativecontext
