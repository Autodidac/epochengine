/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 *                                                            *
 *   This file is part of the Almond Project.                 *
 *   AlmondShell - Modular C++ Framework                      *
 *                                                            *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
 *                                                            *
 *   Provided "AS IS", without warranty of any kind.          *
 *   Use permitted for Non-Commercial Purposes ONLY,          *
 *   without prior commercial licensing agreement.            *
 *                                                            *
 *   Redistribution Allowed with This Notice and              *
 *   LICENSE file. No obligation to disclose modifications.   *
 *                                                            *
 *   See LICENSE file for full terms.                         *
 *                                                            *
 **************************************************************/
 // afontrenderer.hpp
#pragma once

#include "aplatform.hpp"
// #include "aassetsystem.hpp" // uncomment when asset system is ready
#include "aatlastexture.hpp"
#include "arenderer.hpp"
#include "agui.hpp"

#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace almondnamespace::font
{
    struct UVRect
    {
        ui::vec2 top_left{};
        ui::vec2 bottom_right{};
    };

    struct Glyph
    {
        UVRect uv{};
        ui::vec2 size_px{};     // Glyph pixel size
        ui::vec2 offset_px{};   // Offset from baseline in pixels
        float advance{};        // Cursor advance in pixels after rendering this glyph
    };

    struct FontAsset
    {
        std::string name;
        float size_pt{};

        std::unordered_map<char32_t, Glyph> glyphs;
        AtlasEntry atlas; // lightweight handle to the glyph atlas texture
    };

    class FontRenderer
    {
    public:
        // Load and bake font from TTF file into a glyph atlas
        // Returns false on failure, true on success
        bool load_font(const std::string& name, const std::string& ttf_path, float size_pt)
        {
            FontAsset asset{};
            asset.name = name;
            asset.size_pt = size_pt;

            std::vector<std::pair<char32_t, BakedGlyph>> baked_glyphs{};
            Texture raw_texture{};

            if (!load_and_bake_font(ttf_path, size_pt, baked_glyphs, raw_texture))
            {
                std::cerr << "[FontRenderer] Failed to bake font '" << name << "' from '" << ttf_path << "'\n";
                return false;
            }

            if (raw_texture.empty())
            {
                std::cerr << "[FontRenderer] Baked texture for font '" << name << "' is empty\n";
                return false;
            }

            raw_texture.name = name;

            static std::mutex atlas_mutex;
            static almondnamespace::TextureAtlas atlas = almondnamespace::TextureAtlas::create({
                .name = "font_atlas",
                .width = 2048,
                .height = 2048,
                .generate_mipmaps = false
                });

            std::optional<AtlasEntry> maybe_entry;
            {
                std::lock_guard<std::mutex> lock(atlas_mutex);
                maybe_entry = atlas.add_entry(name, raw_texture);
            }

            if (!maybe_entry)
            {
                std::cerr << "[FontRenderer] Failed to pack font '" << name << "' into shared atlas\n";
                return false;
            }

            const AtlasEntry& atlas_entry = *maybe_entry;
            const float inv_atlas_width = atlas.width > 0 ? 1.0f / static_cast<float>(atlas.width) : 0.0f;
            const float inv_atlas_height = atlas.height > 0 ? 1.0f / static_cast<float>(atlas.height) : 0.0f;

            for (auto& [codepoint, baked] : baked_glyphs)
            {
                Glyph glyph = std::move(baked.glyph);

                const float glyph_left = static_cast<float>(atlas_entry.region.x + baked.x0);
                const float glyph_top = static_cast<float>(atlas_entry.region.y + baked.y0);
                const float glyph_right = static_cast<float>(atlas_entry.region.x + baked.x1);
                const float glyph_bottom = static_cast<float>(atlas_entry.region.y + baked.y1);

                glyph.uv.top_left = {
                    glyph_left * inv_atlas_width,
                    1.0f - glyph_top * inv_atlas_height
                };
                glyph.uv.bottom_right = {
                    glyph_right * inv_atlas_width,
                    1.0f - glyph_bottom * inv_atlas_height
                };
                asset.glyphs.emplace(codepoint, std::move(glyph));
            }

            asset.atlas = atlas_entry;
            loaded_fonts_.emplace(name, std::move(asset));
            return true;
        }


        // Render UTF-32 text at pixel coordinates in screen space
        void render_text(const std::string& font_name, const std::u32string& text, ui::vec2 pos_px) const
        {
            auto it = loaded_fonts_.find(font_name);
            if (it == loaded_fonts_.end())
                return; // font not loaded, silently drop text (optionally log warning)

            const FontAsset& font = it->second;
            ui::vec2 cursor = pos_px;

            for (char32_t ch : text)
            {
                auto glyph_it = font.glyphs.find(ch);
                if (glyph_it == font.glyphs.end())
                {
                    // Optional: render fallback glyph or log missing glyph here
                    continue;
                }

                const Glyph& g = glyph_it->second;
                ui::vec2 top_left = cursor + g.offset_px;
                ui::vec2 bottom_right = top_left + g.size_px;

                almondnamespace::Renderer::submit_quad(
                    top_left,
                    bottom_right,
                    font.atlas.name,
                    g.uv.top_left,
                    g.uv.bottom_right
                );

                cursor.x += g.advance;
            }
        }

    private:
        struct BakedGlyph
        {
            Glyph glyph{};
            int x0 = 0;
            int y0 = 0;
            int x1 = 0;
            int y1 = 0;
        };

        bool load_and_bake_font(const std::string& ttf_path,
            float size_pt,
            std::vector<std::pair<char32_t, BakedGlyph>>& out_glyphs,
            Texture& out_texture);

        std::unordered_map<std::string, FontAsset> loaded_fonts_{};
    };

} // namespace almond::font
