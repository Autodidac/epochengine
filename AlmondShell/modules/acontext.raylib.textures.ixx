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
module;

#include "aengine.config.hpp"

#if defined(ALMOND_USING_RAYLIB)
#include <raylib.h>
#endif

export module acontext.raylib.textures;

import <atomic>;
import <cstdint>;
import <filesystem>;
import <format>;
import <fstream>;
import <iostream>;
import <mutex>;
import <string>;
import <string_view>;
import <unordered_map>;
import <utility>;
import <vector>;

import aengine.core.context;   // core::get_current_render_context()
import aatlas.manager;         // atlasmanager::ensure_uploaded()
import aatlas.texture;         // TextureAtlas, AtlasRegion
import aimage.loader;          // ImageData
import atexture;               // Texture (CPU texture struct)

#if defined(ALMOND_USING_RAYLIB)

export namespace almondnamespace::raylibtextures
{
    using Handle = std::uint32_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    struct AtlasGPU
    {
        Texture2D texture{};
        u64 version = static_cast<u64>(-1);
        u32 width = 0;
        u32 height = 0;
    };

    struct TextureAtlasPtrHash
    {
        size_t operator()(const TextureAtlas* atlas) const noexcept
        {
            return std::hash<const TextureAtlas*>{}(atlas);
        }
    };

    struct TextureAtlasPtrEqual
    {
        bool operator()(const TextureAtlas* lhs, const TextureAtlas* rhs) const noexcept
        {
            return lhs == rhs;
        }
    };

    struct BackendData
    {
        std::unordered_map<const TextureAtlas*, AtlasGPU, TextureAtlasPtrHash, TextureAtlasPtrEqual> gpu_atlases;
        std::mutex gpuMutex;
    };

    // ---------------------------------------------------------------------
    // Per-render-context backend data
    // ---------------------------------------------------------------------
    inline BackendData& get_raylib_backend()
    {
        // Must be called on a render thread (MultiContextManager sets it).
        auto ctx = almondnamespace::core::get_current_render_context();
        if (!ctx)
            throw std::runtime_error("[RaylibTextures] No current render context");

        // Lazily allocate backend data per-context.
        // NOTE: ownership is manual; see shutdown_current_context_backend().
        if (!ctx->native_drawable)
            ctx->native_drawable = new BackendData();

        return *static_cast<BackendData*>(ctx->native_drawable);
    }

    // Call this from your raylib backend cleanup path (once per context/thread).
    export inline void shutdown_current_context_backend() noexcept
    {
        try
        {
            auto& backend = get_raylib_backend();
            std::scoped_lock lock(backend.gpuMutex);
            for (auto& [_, gpu] : backend.gpu_atlases)
            {
                if (gpu.texture.id != 0)
                    UnloadTexture(gpu.texture);
            }
            backend.gpu_atlases.clear();

            // also delete the per-context allocation
            if (auto ctx = almondnamespace::core::get_current_render_context(); ctx && ctx->native_drawable)
            {
                delete static_cast<BackendData*>(ctx->native_drawable);
                ctx->native_drawable = nullptr;
            }
        }
        catch (...) {}
    }

    // ---------------------------------------------------------------------
    // Debug dumping
    // ---------------------------------------------------------------------
    inline std::atomic_uint32_t s_dumpSerial{ 0 };

    inline std::string make_dump_name(int atlasIdx, std::string_view tag)
    {
        std::filesystem::create_directories("atlas_dump");
        return std::format("atlas_dump/{}_{}_{}.ppm",
            tag,
            atlasIdx,
            s_dumpSerial.fetch_add(1, std::memory_order_relaxed));
    }

    inline void dump_atlas_rgb_ppm(const TextureAtlas& atlas, int atlasIdx)
    {
        const std::string filename = make_dump_name(atlasIdx, atlas.name);

        std::ofstream out(filename, std::ios::binary);
        if (!out)
        {
            std::cerr << "[Dump] Failed to open: " << filename << "\n";
            return;
        }

        out << "P6\n" << atlas.width << " " << atlas.height << "\n255\n";

        const auto& px = atlas.pixel_data; // RGBA
        for (std::size_t i = 0; i + 2 < px.size(); i += 4)
        {
            out.put(static_cast<char>(px[i + 0]));
            out.put(static_cast<char>(px[i + 1]));
            out.put(static_cast<char>(px[i + 2]));
        }

        std::cerr << "[Dump] Wrote: " << filename << "\n";
    }

    // ---------------------------------------------------------------------
    // Format conversion
    // ---------------------------------------------------------------------
    [[nodiscard]]
    inline ImageData ensure_rgba(const ImageData& img)
    {
        const std::size_t pixelCount =
            static_cast<std::size_t>(img.width) * static_cast<std::size_t>(img.height);

        if (pixelCount == 0)
            return img;

        const std::size_t channels = img.pixels.size() / pixelCount;

        if (channels == 4)
            return img;

        if (channels != 3)
            throw std::runtime_error("ensure_rgba(): Unsupported channel count: " + std::to_string(channels));

        std::vector<std::uint8_t> rgba(pixelCount * 4);
        const std::uint8_t* src = img.pixels.data();

        for (std::size_t i = 0; i < pixelCount; ++i)
        {
            rgba[4 * i + 0] = src[3 * i + 0];
            rgba[4 * i + 1] = src[3 * i + 1];
            rgba[4 * i + 2] = src[3 * i + 2];
            rgba[4 * i + 3] = 255;
        }

        return { std::move(rgba), img.width, img.height, 4 };
    }

    // ---------------------------------------------------------------------
    // GPU upload (private helper; caller must hold backend.gpuMutex)
    // ---------------------------------------------------------------------
    inline void upload_atlas_to_gpu_locked(BackendData& backend, const TextureAtlas& atlas)
    {
        if (atlas.pixel_data.empty())
            const_cast<TextureAtlas&>(atlas).rebuild_pixels();

        AtlasGPU& gpu = backend.gpu_atlases[&atlas];

        if (gpu.version == atlas.version && gpu.texture.id != 0)
            return;

        if (gpu.texture.id != 0)
            UnloadTexture(gpu.texture);

        Image img{};
        img.data = const_cast<unsigned char*>(atlas.pixel_data.data());
        img.width = atlas.width;
        img.height = atlas.height;
        img.mipmaps = 1;
        img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

        gpu.texture = LoadTextureFromImage(img);
        gpu.version = atlas.version;
        gpu.width = static_cast<u32>(atlas.width);
        gpu.height = static_cast<u32>(atlas.height);

        // Optional debug
        dump_atlas_rgb_ppm(atlas, atlas.index);

        std::cerr << "[Raylib] Uploaded atlas '" << atlas.name
            << "' (tex id " << gpu.texture.id
            << ", version " << gpu.version << ")\n";
    }

    // ---------------------------------------------------------------------
    // EXPORTED API used by renderer
    // ---------------------------------------------------------------------
    export inline void ensure_uploaded(const TextureAtlas& atlas)
    {
        // Thread-safe on the render thread: per-context backend storage + mutex.
        auto& backend = get_raylib_backend();
        std::scoped_lock lock(backend.gpuMutex);
        upload_atlas_to_gpu_locked(backend, atlas);
    }

    export inline const Texture2D* try_get_texture(const TextureAtlas& atlas) noexcept
    {
        try
        {
            auto& backend = get_raylib_backend();
            std::scoped_lock lock(backend.gpuMutex);

            auto it = backend.gpu_atlases.find(&atlas);
            if (it == backend.gpu_atlases.end())
                return nullptr;

            if (it->second.texture.id == 0)
                return nullptr;

            return &it->second.texture;
        }
        catch (...)
        {
            return nullptr;
        }
    }

    export inline void clear_gpu_atlases() noexcept
    {
        try
        {
            auto& backend = get_raylib_backend();
            std::scoped_lock lock(backend.gpuMutex);

            for (auto& [_, gpu] : backend.gpu_atlases)
            {
                if (gpu.texture.id != 0)
                    UnloadTexture(gpu.texture);
            }

            backend.gpu_atlases.clear();
        }
        catch (...)
        {
        }
    }

    // ---------------------------------------------------------------------
    // Helpers kept for your existing handle / atlas add path
    // ---------------------------------------------------------------------
    inline std::atomic_uint8_t s_generation{ 1 };

    [[nodiscard]]
    inline Handle make_handle(int atlasIdx, int localIdx) noexcept
    {
        return (Handle(s_generation.load(std::memory_order_relaxed)) << 24)
            | ((Handle(atlasIdx) & 0xFFFu) << 12)
            | (Handle(localIdx) & 0xFFFu);
    }

    [[nodiscard]]
    inline bool is_handle_live(Handle h) noexcept
    {
        return std::uint8_t(h >> 24) == s_generation.load(std::memory_order_relaxed);
    }

    export inline Handle load_atlas(const TextureAtlas& atlas, int atlasIndex = -1)
    {
        // Engine-level “ensure uploaded” (may call backend hooks depending on your manager)
        atlasmanager::ensure_uploaded(atlas);

        const int resolvedIndex = (atlasIndex >= 0) ? atlasIndex : atlas.get_index();
        return make_handle(resolvedIndex, 0);
    }

    export inline Handle atlas_add_texture(TextureAtlas& atlas, const std::string& id, const ImageData& img)
    {
        // NOT const: we want to move pixels into Texture.
        auto rgba = ensure_rgba(img);

        Texture texture{
            .width = static_cast<std::uint32_t>(rgba.width),
            .height = static_cast<std::uint32_t>(rgba.height),
            .pixels = std::move(rgba.pixels)
        };

        auto addedOpt = atlas.add_entry(id, texture);
        if (!addedOpt)
            throw std::runtime_error("atlas_add_texture: Failed to add texture: " + id);

        atlasmanager::ensure_uploaded(atlas);
        return make_handle(atlas.get_index(), addedOpt->index);
    }



} // namespace almondnamespace::raylibtextures

#endif // ALMOND_USING_RAYLIB
