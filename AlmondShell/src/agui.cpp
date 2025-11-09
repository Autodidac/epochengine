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
 // agui.cpp
#include "pch.h"

#include "agui.hpp"

#include "aatlasmanager.hpp"
#include "acontext.hpp"
#include "aspritepool.hpp"
#include "aspriteregistry.hpp"
#include "atexture.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace almondnamespace::gui
{
namespace
{
    using Vec2 = almondnamespace::gui::Vec2;
    using Color = ::almondnamespace::gui::Color;
    using EventType = almondnamespace::gui::EventType;
    using InputEvent = ::almondnamespace::gui::InputEvent;

    // ASCII glyphs 0x20-0x7F from the public domain font8x8_basic set by Daniel Hepper.
    constexpr std::array<std::array<std::uint8_t, 8>, 96> kFont8x8Basic = { {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // space
        { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00 }, // !
        { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // "
        { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00 }, // #
        { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00 }, // $
        { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00 }, // %
        { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00 }, // &
        { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '
        { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00 }, // (
        { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00 }, // )
        { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 }, // *
        { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00 }, // +
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // ,
        { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00 }, // -
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // .
        { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00 }, // /
        { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00 }, // 0
        { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00 }, // 1
        { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00 }, // 2
        { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00 }, // 3
        { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00 }, // 4
        { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00 }, // 5
        { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00 }, // 6
        { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00 }, // 7
        { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00 }, // 8
        { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00 }, // 9
        { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // :
        { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // ;
        { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00 }, // <
        { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00 }, // =
        { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00 }, // >
        { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00 }, // ?
        { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00 }, // @
        { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00 }, // A
        { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00 }, // B
        { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00 }, // C
        { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00 }, // D
        { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00 }, // E
        { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00 }, // F
        { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00 }, // G
        { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00 }, // H
        { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // I
        { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00 }, // J
        { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00 }, // K
        { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00 }, // L
        { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00 }, // M
        { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00 }, // N
        { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00 }, // O
        { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00 }, // P
        { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00 }, // Q
        { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00 }, // R
        { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00 }, // S
        { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // T
        { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00 }, // U
        { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // V
        { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00 }, // W
        { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00 }, // X
        { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00 }, // Y
        { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00 }, // Z
        { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00 }, // [
        { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00 }, // \

        { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00 }, // ]
        { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00 }, // ^
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF }, // _
        { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 }, // `
        { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00 }, // a
        { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00 }, // b
        { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00 }, // c
        { 0x38, 0x30, 0x30, 0x3E, 0x33, 0x33, 0x6E, 0x00 }, // d
        { 0x00, 0x00, 0x1E, 0x33, 0x3F, 0x03, 0x1E, 0x00 }, // e
        { 0x1C, 0x36, 0x06, 0x0F, 0x06, 0x06, 0x0F, 0x00 }, // f
        { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // g
        { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00 }, // h
        { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // i
        { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E }, // j
        { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00 }, // k
        { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // l
        { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00 }, // m
        { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00 }, // n
        { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00 }, // o
        { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F }, // p
        { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78 }, // q
        { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00 }, // r
        { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00 }, // s
        { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00 }, // t
        { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00 }, // u
        { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // v
        { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00 }, // w
        { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00 }, // x
        { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // y
        { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00 }, // z
        { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00 }, // {
        { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 }, // |
        { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00 }, // }
        { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ~
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }  // DEL
    } };

    struct GuiResources {
        bool atlasBuilt = false;
        TextureAtlas* atlas = nullptr;
        SpriteHandle windowBackground{};
        SpriteHandle buttonNormal{};
        SpriteHandle buttonHover{};
        SpriteHandle buttonActive{};
        SpriteHandle textField{};
        SpriteHandle textFieldActive{};
        SpriteHandle panelBackground{};
        SpriteHandle consoleBackground{};
        std::array<SpriteHandle, 96> glyphs{};
        float glyphWidth = 8.0f;
        float glyphHeight = 8.0f;
    };

    GuiResources g_resources{};
    std::mutex g_resourceMutex;

    struct PtrHash {
        size_t operator()(const void* p) const noexcept {
            return std::hash<const void*>{}(p);
        }
    };
    static std::unordered_set<const void*, PtrHash> g_uploadedContexts;
    static std::mutex g_uploadMutex;

    struct FrameState {
        // thread's active render context (must be set in begin_frame)
        std::shared_ptr<core::Context> ctxShared{};
        core::Context* ctx = nullptr;

        Vec2 cursor{};
        Vec2 origin{};
        Vec2 windowSize{};
        Vec2 mousePos{};

        bool mouseDown = false;
        bool prevMouseDown = false;
        bool justReleased = false;
        bool insideWindow = false;
        bool justPressed = false;

        std::optional<WidgetBounds> lastButtonBounds{};

        float deltaTime = 0.0f;
        float caretTimer = 0.0f;
        bool caretVisible = true;

        std::vector<InputEvent> events{};
    };

    thread_local FrameState g_frame{};
    thread_local std::vector<InputEvent> g_pendingEvents{};
    thread_local const void* g_activeWidget = nullptr;

    // Cheap tripwire so this doesn't regress again:
    static_assert(std::is_same_v<decltype(g_frame.ctx), core::Context*>,
        "FrameState must carry core::Context* ctx");

    constexpr const char* kAtlasName = "__agui_builtin";
    constexpr float kContentPadding = 8.0f;
    constexpr float kLineSpacing = 4.0f;
    constexpr float kFontScale = 2.0f;
    constexpr float kTitleScale = 2.5f;
    constexpr float kBoxInnerPadding = 6.0f;
    constexpr float kCaretBlinkPeriod = 1.0f;

    [[nodiscard]] std::vector<std::uint8_t> make_solid_pixels(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a, std::uint32_t w, std::uint32_t h)
    {
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w) * h * 4, 0);
        for (std::size_t idx = 0; idx < pixels.size(); idx += 4) {
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = a;
        }
        return pixels;
    }

    [[nodiscard]] std::vector<std::uint8_t> make_glyph_pixels(const std::array<std::uint8_t, 8>& glyph)
    {
        constexpr std::uint32_t w = 8;
        constexpr std::uint32_t h = 8;
        std::vector<std::uint8_t> pixels(w * h * 4, 0);
        for (std::uint32_t row = 0; row < h; ++row) {
            const std::uint8_t bits = glyph[row];
            for (std::uint32_t col = 0; col < w; ++col) {
                const bool on = (bits >> col) & 0x1u;
                const std::size_t index = static_cast<std::size_t>(row) * w * 4 + col * 4;
                pixels[index + 0] = 255;
                pixels[index + 1] = 255;
                pixels[index + 2] = 255;
                pixels[index + 3] = on ? 255 : 0;
            }
        }
        return pixels;
    }

    [[nodiscard]] SpriteHandle add_sprite(TextureAtlas& atlas, const std::string& name, const std::vector<std::uint8_t>& pixels, std::uint32_t w, std::uint32_t h)
    {
        Texture texture{};
        texture.name = name;
        texture.width = w;
        texture.height = h;
        texture.channels = 4;
        texture.pixels = pixels;

        auto entry = atlas.add_entry(name, texture);
        if (!entry) {
            throw std::runtime_error("[agui] Failed to add atlas entry: " + name);
        }

        if (almondnamespace::spritepool::capacity == 0) {
            almondnamespace::spritepool::initialize(2048);
        }

        SpriteHandle handle = almondnamespace::spritepool::allocate();
        if (!handle.is_valid()) {
            throw std::runtime_error("[agui] Sprite pool exhausted while registering GUI sprite");
        }

        handle.atlasIndex = static_cast<std::uint32_t>(atlas.get_index());
        handle.localIndex = static_cast<std::uint32_t>(entry->index);

        almondnamespace::atlasmanager::registry.add(name, handle,
            entry->region.u1,
            entry->region.v1,
            entry->region.u2 - entry->region.u1,
            entry->region.v2 - entry->region.v1);

        return handle;
    }

    void ensure_resources()
    {
        std::scoped_lock lock(g_resourceMutex);
        if (g_resources.atlasBuilt)
            return;

        auto atlasIt = almondnamespace::atlasmanager::atlas_map.find(kAtlasName);
        if (atlasIt == almondnamespace::atlasmanager::atlas_map.end()) {
            almondnamespace::atlasmanager::create_atlas({
                .name = kAtlasName,
                .width = 512,
                .height = 512,
                .generate_mipmaps = false
            });
            atlasIt = almondnamespace::atlasmanager::atlas_map.find(kAtlasName);
            if (atlasIt == almondnamespace::atlasmanager::atlas_map.end()) {
                throw std::runtime_error("[agui] Unable to create GUI atlas");
            }
        }

        TextureAtlas& atlas = atlasIt->second;
        g_resources.atlas = &atlas;

        g_resources.windowBackground = add_sprite(atlas, "__agui/window_bg",
            make_solid_pixels(0x33, 0x35, 0x38, 0xFF, 8, 8), 8, 8);
        g_resources.buttonNormal = add_sprite(atlas, "__agui/button_normal",
            make_solid_pixels(0x5B, 0x5F, 0x66, 0xFF, 8, 8), 8, 8);
        g_resources.buttonHover = add_sprite(atlas, "__agui/button_hover",
            make_solid_pixels(0x76, 0x7C, 0x85, 0xFF, 8, 8), 8, 8);
        g_resources.buttonActive = add_sprite(atlas, "__agui/button_active",
            make_solid_pixels(0x94, 0x9A, 0xA3, 0xFF, 8, 8), 8, 8);
        g_resources.textField = add_sprite(atlas, "__agui/text_field",
            make_solid_pixels(0x2B, 0x2E, 0x33, 0xFF, 8, 8), 8, 8);
        g_resources.textFieldActive = add_sprite(atlas, "__agui/text_field_active",
            make_solid_pixels(0x3A, 0x3E, 0x45, 0xFF, 8, 8), 8, 8);
        g_resources.panelBackground = add_sprite(atlas, "__agui/panel_bg",
            make_solid_pixels(0x27, 0x29, 0x2E, 0xFF, 8, 8), 8, 8);
        g_resources.consoleBackground = add_sprite(atlas, "__agui/console_bg",
            make_solid_pixels(0x1F, 0x21, 0x26, 0xFF, 8, 8), 8, 8);

        for (std::size_t i = 0; i < kFont8x8Basic.size(); ++i) {
            const int codepoint = static_cast<int>(i) + 32;
            const std::string name = "__agui/glyph_" + std::to_string(codepoint);
            auto pixels = make_glyph_pixels(kFont8x8Basic[i]);
            g_resources.glyphs[i] = add_sprite(atlas, name, pixels, 8, 8);
        }

        g_resources.atlasBuilt = true;
    }

    void ensure_backend_upload(core::Context& ctx)
    {
        ensure_resources();
        if (!g_resources.atlas) return;

        std::scoped_lock lock(g_uploadMutex);
        if (g_uploadedContexts.contains(&ctx)) return;

        const std::uint32_t handle = ctx.add_atlas_safe(*g_resources.atlas);
        if (handle == 0) {
            throw std::runtime_error("[agui] Failed to upload GUI atlas for this context");
        }
        g_uploadedContexts.insert(&ctx);
    }

    void draw_sprite(const SpriteHandle& handle, float x, float y, float w, float h)
    {
        if (!handle.is_valid())
            return;

        core::Context* ctx = g_frame.ctx;
        if (!ctx)
            return;

        bool queued = false;
        if (ctx->windowData && g_frame.ctxShared) {
            auto ctxShared = g_frame.ctxShared;
            ctx->windowData->commandQueue.enqueue([ctxShared,
                handle,
                x,
                y,
                w,
                h]() {
                    if (!ctxShared)
                        return;
                    const auto& atlasesRT = almondnamespace::atlasmanager::get_atlas_vector();
                    ctxShared->draw_sprite_safe(handle, atlasesRT, x, y, w, h);
                });
            queued = true;
        }

        if (!queued) {
            const auto& atlases = almondnamespace::atlasmanager::get_atlas_vector();
            ctx->draw_sprite_safe(handle, atlases, x, y, w, h);
        }
    }

    [[nodiscard]] SpriteHandle glyph_handle(unsigned char ch) noexcept
    {
        if (ch >= 32 && ch < 128) {
            return g_resources.glyphs[ch - 32];
        }
        return g_resources.glyphs['?' - 32];
    }

    [[nodiscard]] bool point_in_rect(Vec2 point, float x, float y, float w, float h) noexcept
    {
        return (point.x >= x && point.x <= x + w && point.y >= y && point.y <= y + h);
    }

    [[nodiscard]] float measure_wrapped_text_height(std::string_view text, float width, float scale) noexcept
    {
        ensure_resources();
        const float glyphW = g_resources.glyphWidth * scale;
        const float glyphH = g_resources.glyphHeight * scale;
        const float effectiveWidth = std::max(glyphW, width);

        std::size_t lines = 1;
        float penX = 0.0f;
        for (char ch : text) {
            if (ch == '\n') {
                ++lines;
                penX = 0.0f;
                continue;
            }

            if (penX + glyphW > effectiveWidth + 0.001f) {
                ++lines;
                penX = 0.0f;
            }

            penX += glyphW;
        }

        const float totalHeight = static_cast<float>(lines) * glyphH + static_cast<float>(lines - 1) * kLineSpacing;
        return std::max(glyphH, totalHeight);
    }

    [[nodiscard]] almondnamespace::gui::Vec2 compute_caret_position(std::string_view text, float x, float y, float width, float scale) noexcept
    {
        ensure_resources();
        const float glyphW = g_resources.glyphWidth * scale;
        const float glyphH = g_resources.glyphHeight * scale;
        const float effectiveWidth = std::max(glyphW, width);

        float startX = x;
        float penX = x;
        float penY = y;

        for (char ch : text) {
            if (ch == '\n') {
                penX = startX;
                penY += glyphH + kLineSpacing;
                continue;
            }

            if (penX + glyphW > startX + effectiveWidth + 0.001f) {
                penX = startX;
                penY += glyphH + kLineSpacing;
            }

            penX += glyphW;
        }

        return {penX, penY};
    }

    float draw_wrapped_text(std::string_view text, float x, float y, float width, float scale)
    {
        ensure_resources();
        if (!g_frame.ctx)
            return 0.0f;

        const float glyphW = g_resources.glyphWidth * scale;
        const float glyphH = g_resources.glyphHeight * scale;
        const float effectiveWidth = std::max(glyphW, width);

        const float startX = x;
        float penX = x;
        float penY = y;

        for (char ch : text) {
            if (ch == '\n') {
                penX = startX;
                penY += glyphH + kLineSpacing;
                continue;
            }

            if (penX + glyphW > startX + effectiveWidth + 0.001f) {
                penX = startX;
                penY += glyphH + kLineSpacing;
            }

            const SpriteHandle handle = glyph_handle(static_cast<unsigned char>(ch));
            draw_sprite(handle, penX, penY, glyphW, glyphH);
            penX += glyphW;
        }

        return (penY - y) + glyphH;
    }

    void draw_caret(float x, float y, float height)
    {
        const float caretWidth = std::max(1.0f, g_resources.glyphWidth * kFontScale * 0.1f);
        draw_sprite(g_resources.buttonActive, x, y, caretWidth, height);
    }

    void draw_text_line(std::string_view text, float x, float y, float scale)
    {
        ensure_resources();
        if (!g_frame.ctx)
            return;

        const float glyphW = g_resources.glyphWidth * scale;
        const float glyphH = g_resources.glyphHeight * scale;

        for (char ch : text) {
            if (ch == '\n') {
                y += glyphH + kLineSpacing;
                x = g_frame.origin.x + kContentPadding;
                continue;
            }

            const SpriteHandle handle = glyph_handle(static_cast<unsigned char>(ch));
            draw_sprite(handle, x, y, glyphW, glyphH);
            x += glyphW;
        }
    }

    void reset_frame()
    {
        g_frame.cursor = {};
        g_frame.origin = {};
        g_frame.windowSize = {};
        g_frame.insideWindow = false;
        g_frame.lastButtonBounds.reset();
    }


} // namespace

void set_cursor(Vec2 position) noexcept
{
    if (!g_frame.insideWindow)
        return;

    g_frame.cursor = position;
}

void advance_cursor(Vec2 delta) noexcept
{
    if (!g_frame.insideWindow)
        return;

    g_frame.cursor += delta;
}

void push_input(const almondnamespace::gui::InputEvent& e) noexcept
{
    g_pendingEvents.push_back(e);
}

void begin_frame(std::shared_ptr<core::Context> ctx, float dt, Vec2 mouse_pos, bool mouse_down) noexcept
{
    core::Context* rawCtx = ctx.get();
    if (rawCtx) {
        try {
            ensure_backend_upload(*rawCtx);
        }
        catch (...) {
            // swallow: GUI is optional, avoid propagating exceptions across frames
        }
    }

    g_frame.ctxShared = std::move(ctx);
    g_frame.ctx = rawCtx;
    g_frame.deltaTime = dt;

    g_frame.caretTimer += dt;
    while (g_frame.caretTimer >= kCaretBlinkPeriod) {
        g_frame.caretTimer -= kCaretBlinkPeriod;
    }
    g_frame.caretVisible = (g_frame.caretTimer < (kCaretBlinkPeriod * 0.5f));

    const bool prevMouseDown = g_frame.mouseDown;
    bool currentMouseDown = mouse_down;
    Vec2 currentMousePos = mouse_pos;

    g_frame.events.clear();
    if (!g_pendingEvents.empty()) {
        g_frame.events.insert(g_frame.events.end(), g_pendingEvents.begin(), g_pendingEvents.end());
        g_pendingEvents.clear();
    }

    for (const auto& evt : g_frame.events) {
        switch (evt.type) {
        case EventType::MouseMove:
            currentMousePos = evt.mouse_pos;
            break;
        case EventType::MouseDown:
            currentMouseDown = true;
            currentMousePos = evt.mouse_pos;
            break;
        case EventType::MouseUp:
            currentMouseDown = false;
            currentMousePos = evt.mouse_pos;
            break;
        default:
            break;
        }
    }

    g_frame.prevMouseDown = prevMouseDown;
    g_frame.mouseDown = currentMouseDown;
    g_frame.mousePos = currentMousePos;
    g_frame.justPressed = (!prevMouseDown && currentMouseDown);
    g_frame.justReleased = (prevMouseDown && !currentMouseDown);
    reset_frame();
}

void end_frame() noexcept
{
    g_frame.ctxShared.reset();
    g_frame.ctx = nullptr;
    g_frame.insideWindow = false;
    g_frame.events.clear();
    g_frame.justPressed = false;
    g_frame.justReleased = false;
}

void begin_window(std::string_view title, Vec2 position, Vec2 size) noexcept
{
    if (!g_frame.ctx)
        return;

    ensure_resources();

    g_frame.origin = position;
    g_frame.windowSize = size;
    g_frame.insideWindow = true;
    set_cursor({ position.x + kContentPadding, position.y + kContentPadding });

    draw_sprite(g_resources.windowBackground, position.x, position.y, size.x, size.y);

    const float titleHeight = g_resources.glyphHeight * kTitleScale;
    draw_text_line(title, position.x + kContentPadding, position.y + kContentPadding, kTitleScale);

    advance_cursor({ 0.0f, titleHeight + kContentPadding });
}

void end_window() noexcept
{
    g_frame.insideWindow = false;
}

bool button(std::string_view label, Vec2 size) noexcept
{
    if (!g_frame.insideWindow || !g_frame.ctx)
        return false;

    const Vec2 pos = g_frame.cursor;
    const float width = std::max<float>(static_cast<float>(size.x), g_resources.glyphWidth * kFontScale + 2.0f * kContentPadding);
    const float height = std::max<float>(static_cast<float>(size.y), g_resources.glyphHeight * kFontScale + 2.0f * kContentPadding);

    const bool hovered = point_in_rect(g_frame.mousePos, pos.x, pos.y, width, height);

    const SpriteHandle background = hovered
        ? (g_frame.mouseDown ? g_resources.buttonActive : g_resources.buttonHover)
        : g_resources.buttonNormal;

    draw_sprite(background, pos.x, pos.y, width, height);

    const float textWidth = static_cast<float>(label.size()) * g_resources.glyphWidth * kFontScale;
    const float textHeight = g_resources.glyphHeight * kFontScale;
    const float textX = pos.x + std::max(0.0f, (width - textWidth) * 0.5f);
    const float textY = pos.y + std::max(0.0f, (height - textHeight) * 0.5f);
    draw_text_line(label, textX, textY, kFontScale);

    g_frame.lastButtonBounds = WidgetBounds{ pos, { width, height } };
    advance_cursor({ 0.0f, height + kContentPadding });
    return hovered && g_frame.justPressed;
}

bool image_button(const SpriteHandle& sprite, Vec2 size) noexcept
{
    if (!g_frame.insideWindow || !g_frame.ctx)
        return false;

    ensure_resources();

    const Vec2 pos = g_frame.cursor;
    const float minSize = g_resources.glyphHeight * kFontScale * 2.0f;
    const float width = std::max<float>(static_cast<float>(size.x), minSize);
    const float height = std::max<float>(static_cast<float>(size.y), minSize);

    const bool hovered = point_in_rect(g_frame.mousePos, pos.x, pos.y, width, height);
    const SpriteHandle background = hovered
        ? (g_frame.mouseDown ? g_resources.buttonActive : g_resources.buttonHover)
        : g_resources.buttonNormal;

    draw_sprite(background, pos.x, pos.y, width, height);
    if (sprite.is_valid()) {
        draw_sprite(sprite, pos.x, pos.y, width, height);
    }

    g_frame.lastButtonBounds = WidgetBounds{ pos, { width, height } };
    advance_cursor({ 0.0f, height + kContentPadding });
    return hovered && g_frame.justPressed;
}

EditBoxResult edit_box(std::string& text, Vec2 size, std::size_t max_chars, bool multiline) noexcept
{
    EditBoxResult result{};
    if (!g_frame.insideWindow || !g_frame.ctx)
        return result;

    ensure_resources();

    const Vec2 pos = g_frame.cursor;
    const float minWidth = g_resources.glyphWidth * kFontScale * 4.0f;
    const float minHeight = g_resources.glyphHeight * kFontScale + 2.0f * kBoxInnerPadding;
    const float width = std::max<float>(static_cast<float>(size.x), minWidth);
    const float height = std::max<float>(static_cast<float>(size.y), minHeight);

    const bool hovered = point_in_rect(g_frame.mousePos, pos.x, pos.y, width, height);
    const void* id = static_cast<const void*>(&text);

    if (g_frame.justPressed) {
        if (hovered) {
            g_activeWidget = id;
            g_frame.caretTimer = 0.0f;
            g_frame.caretVisible = true;
        }
        else if (g_activeWidget == id) {
            g_activeWidget = nullptr;
        }
    }

    if (g_frame.justReleased && !hovered && g_activeWidget == id) {
        g_activeWidget = nullptr;
    }

    const bool active = (g_activeWidget == id);
    result.active = active;

    const SpriteHandle background = active ? g_resources.textFieldActive : g_resources.textField;
    draw_sprite(background, pos.x, pos.y, width, height);

    const float contentWidth = std::max(1.0f, width - 2.0f * kBoxInnerPadding);
    const float contentHeight = std::max(1.0f, height - 2.0f * kBoxInnerPadding);
    const float textX = pos.x + kBoxInnerPadding;
    const float textY = pos.y + kBoxInnerPadding;

    if (multiline) {
        draw_wrapped_text(text, textX, textY, contentWidth, kFontScale);
    }
    else {
        draw_text_line(text, textX, textY, kFontScale);
    }

    const std::size_t limit = max_chars == 0 ? std::numeric_limits<std::size_t>::max() : max_chars;

    if (active) {
        for (const auto& evt : g_frame.events) {
            switch (evt.type) {
            case EventType::TextInput:
                for (char ch : evt.text) {
                    if (ch == '\r')
                        continue;
                    if (ch == '\n') {
                        if (multiline) {
                            if (text.size() < limit) {
                                text.push_back('\n');
                                result.changed = true;
                            }
                        }
                        else {
                            result.submitted = true;
                        }
                        continue;
                    }
                    if (static_cast<unsigned char>(ch) < 32)
                        continue;
                    if (text.size() < limit) {
                        text.push_back(ch);
                        result.changed = true;
                    }
                }
                break;
            case EventType::KeyDown:
                if (evt.key == 8 || evt.key == 127) {
                    if (!text.empty()) {
                        text.pop_back();
                        result.changed = true;
                    }
                }
                else if (evt.key == 27) {
                    g_activeWidget = nullptr;
                    result.active = false;
                }
                else if (evt.key == 13) {
                    if (multiline) {
                        if (text.size() < limit) {
                            text.push_back('\n');
                            result.changed = true;
                        }
                    }
                    else {
                        result.submitted = true;
                    }
                }
                break;
            default:
                break;
            }
        }
    }

    if (active && g_frame.caretVisible) {
        const Vec2 caret = multiline
            ? compute_caret_position(text, textX, textY, contentWidth, kFontScale)
            : Vec2{ textX + static_cast<float>(text.size()) * g_resources.glyphWidth * kFontScale, textY };

        const float caretHeight = multiline
            ? std::min(contentHeight, g_resources.glyphHeight * kFontScale)
            : g_resources.glyphHeight * kFontScale;

        const float caretRight = textX + std::max(1.0f, contentWidth) - 1.0f;
        const float caretX = std::clamp(caret.x, textX, caretRight);
        const float caretY = std::clamp(caret.y, textY, textY + std::max(0.0f, contentHeight - caretHeight));
        draw_caret(caretX, caretY, caretHeight);
    }

    advance_cursor({ 0.0f, height + kContentPadding });
    return result;
}

void text_box(std::string_view text, Vec2 size) noexcept
{
    if (!g_frame.insideWindow || !g_frame.ctx)
        return;

    ensure_resources();

    const Vec2 pos = g_frame.cursor;
    const float minWidth = g_resources.glyphWidth * kFontScale * 4.0f;
    float width = static_cast<float>(size.x);
    if (width <= 0.0f) {
        const float estimated = static_cast<float>(text.size()) * g_resources.glyphWidth * kFontScale + 2.0f * kBoxInnerPadding;
        width = std::max(minWidth, estimated);
    }
    else {
        width = std::max(width, minWidth);
    }

    const float contentWidth = std::max(1.0f, width - 2.0f * kBoxInnerPadding);
    float height = static_cast<float>(size.y);
    if (height <= 0.0f) {
        const float textHeight = measure_wrapped_text_height(text, contentWidth, kFontScale);
        height = textHeight + 2.0f * kBoxInnerPadding;
    }
    else {
        height = std::max(height, g_resources.glyphHeight * kFontScale + 2.0f * kBoxInnerPadding);
    }

    draw_sprite(g_resources.panelBackground, pos.x, pos.y, width, height);
    draw_wrapped_text(text, pos.x + kBoxInnerPadding, pos.y + kBoxInnerPadding, contentWidth, kFontScale);

    advance_cursor({ 0.0f, height + kContentPadding });
}

ConsoleWindowResult console_window(const ConsoleWindowOptions& options) noexcept
{
    ConsoleWindowResult result{};
    if (!g_frame.ctx)
        return result;

    begin_window(options.title, options.position, options.size);
    if (!g_frame.insideWindow || !g_frame.ctx) {
        end_window();
        return result;
    }

    ensure_resources();

    const float availableWidth = std::max(0.0f, options.size.x - 2.0f * kContentPadding);
    const float logHeight = std::max(0.0f, options.size.y - 3.0f * kContentPadding - (g_resources.glyphHeight * kFontScale));
    const Vec2 logPos = g_frame.cursor;

    if (availableWidth > 0.0f && logHeight > 0.0f) {
        draw_sprite(g_resources.consoleBackground, logPos.x, logPos.y, availableWidth, logHeight);
    }

    const float contentWidth = std::max(1.0f, availableWidth - 2.0f * kBoxInnerPadding);
    float penY = logPos.y + kBoxInnerPadding;
    const float maxY = logPos.y + std::max(0.0f, logHeight - kBoxInnerPadding);

    if (!options.lines.empty() && logHeight > 0.0f) {
        const std::size_t count = options.lines.size();
        const std::size_t start = (count > options.max_visible_lines)
            ? count - options.max_visible_lines
            : 0;
        for (std::size_t i = start; i < count; ++i) {
            const std::string& line = options.lines[i];
            const float drawn = draw_wrapped_text(line, logPos.x + kBoxInnerPadding, penY, contentWidth, kFontScale);
            penY += drawn + kLineSpacing;
            if (penY > maxY) {
                break;
            }
        }
    }

    set_cursor({ logPos.x, logPos.y + logHeight + kContentPadding });

    if (options.input) {
        const float fieldHeight = g_resources.glyphHeight * kFontScale + 2.0f * kBoxInnerPadding;
        Vec2 inputSize{ availableWidth, fieldHeight };
        result.input = edit_box(*options.input, inputSize, options.max_input_chars, options.multiline_input);
    }

    end_window();
    return result;
}

void label(std::string_view text) noexcept
{
    if (!g_frame.insideWindow || !g_frame.ctx)
        return;

    draw_text_line(text, g_frame.cursor.x, g_frame.cursor.y, kFontScale);
    advance_cursor({ 0.0f, g_resources.glyphHeight * kFontScale + kLineSpacing });
}

float line_height() noexcept
{
    try {
        ensure_resources();
    }
    catch (...) {
        return 16.0f;
    }
    return g_resources.glyphHeight * kFontScale + kLineSpacing;
}

float glyph_width() noexcept
{
    try {
        ensure_resources();
    }
    catch (...) {
        return 8.0f * kFontScale;
    }
    return g_resources.glyphWidth * kFontScale;
}

std::optional<WidgetBounds> last_button_bounds() noexcept
{
    return g_frame.lastButtonBounds;
}

} // namespace almondnamespace::gui

