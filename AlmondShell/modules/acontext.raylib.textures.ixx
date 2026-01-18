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
 **************************************************************/
module;

#include "aengine.config.hpp"

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

import aengine.core.context;
import aatlas.manager;
import aatlas.texture;
import aimage.loader;
import atexture;
import acontext.raylib.api;

#if defined(ALMOND_USING_RAYLIB)

export namespace almondnamespace::raylibtextures
{
    using Handle = std::uint32_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    struct AtlasGPU
    {
        almondnamespace::raylib_api::Texture2D texture{};
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

    inline BackendData& get_raylib_backend()
    {
        auto ctx = almondnamespace::core::get_current_render_context();
        if (!ctx)
            throw std::runtime_error("[RaylibTextures] No current render context");

        if (!ctx->native_drawable)
            ctx->native_drawable = new BackendData();

        return *static_cast<BackendData*>(ctx->native_drawable);
    }

    export inline void shutdown_current_context_backend() noexcept
    {
        try
        {
            auto& backend = get_raylib_backend();
            std::scoped_lock lock(backend.gpuMutex);

            for (auto& [_, gpu] : backend.gpu_atlases)
            {
                if (gpu.texture.id != 0)
                    almondnamespace::raylib_api::unload_texture(gpu.texture);
            }
            backend.gpu_atlases.clear();

            if (auto ctx = almondnamespace::core::get_current_render_context(); ctx && ctx->native_drawable)
            {
                delete static_cast<BackendData*>(ctx->native_drawable);
                ctx->native_drawable = nullptr;
            }
        }
        catch (...) {}
    }

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

    inline void upload_atlas_to_gpu_locked(BackendData& backend, const TextureAtlas& atlas)
    {
        if (atlas.pixel_data.empty())
            const_cast<TextureAtlas&>(atlas).rebuild_pixels();

        AtlasGPU& gpu = backend.gpu_atlases[&atlas];

        if (gpu.version == atlas.version && gpu.texture.id != 0)
            return;

        if (gpu.texture.id != 0)
            almondnamespace::raylib_api::unload_texture(gpu.texture);

        almondnamespace::raylib_api::Image img{};
        img.data = const_cast<unsigned char*>(atlas.pixel_data.data());
        img.width = atlas.width;
        img.height = atlas.height;
        img.mipmaps = 1;
        img.format = almondnamespace::raylib_api::pixelformat_rgba8;

        gpu.texture = almondnamespace::raylib_api::load_texture_from_image(img);
        gpu.version = atlas.version;
        gpu.width = static_cast<u32>(atlas.width);
        gpu.height = static_cast<u32>(atlas.height);

        dump_atlas_rgb_ppm(atlas, atlas.index);

        std::cerr << "[Raylib] Uploaded atlas '" << atlas.name
            << "' (tex id " << gpu.texture.id
            << ", version " << gpu.version << ")\n";
    }

    export inline void ensure_uploaded(const TextureAtlas& atlas)
    {
        auto& backend = get_raylib_backend();
        std::scoped_lock lock(backend.gpuMutex);
        upload_atlas_to_gpu_locked(backend, atlas);
    }

    export inline const almondnamespace::raylib_api::Texture2D* try_get_texture(const TextureAtlas& atlas) noexcept
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
                    almondnamespace::raylib_api::unload_texture(gpu.texture);
            }

            backend.gpu_atlases.clear();
        }
        catch (...) {}
    }

    inline std::atomic_uint8_t s_generation{ 1 };

    [[nodiscard]]
    inline Handle make_handle(int atlasIdx, int localIdx) noexcept
    {
        return (Handle(s_generation.load(std::memory_order_relaxed)) << 24)
            | ((Handle(atlasIdx) & 0xFFFu) << 12)
            | (Handle(localIdx) & 0xFFFu);
    }

    export inline Handle load_atlas(const TextureAtlas& atlas, int atlasIndex = -1)
    {
        atlasmanager::ensure_uploaded(atlas);
        const int resolvedIndex = (atlasIndex >= 0) ? atlasIndex : atlas.get_index();
        return make_handle(resolvedIndex, 0);
    }

    export inline Handle atlas_add_texture(TextureAtlas& atlas, const std::string& id, const ImageData& img)
    {
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
}

#endif // ALMOND_USING_RAYLIB
