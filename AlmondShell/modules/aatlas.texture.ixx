/**************************************************************
 *   AlmondShell - Modular C++ Framework
 **************************************************************/

module;

export module aatlas.texture;

// ────────────────────────────────────────────────────────────
// STANDARD LIBRARY IMPORTS
// ────────────────────────────────────────────────────────────

import <string>;
import <vector>;
import <cstdint>;
import <optional>;
import <iostream>;
import <algorithm>;
import <mutex>;
import <shared_mutex>;
import <unordered_map>;
import <memory>;    // std::unique_ptr
import <utility>;   // std::pair

// ────────────────────────────────────────────────────────────
// ENGINE DEPENDENCIES
// ────────────────────────────────────────────────────────────

import atexture;        // provides Texture

// ────────────────────────────────────────────────────────────
// MODULE EXPORTS
// ────────────────────────────────────────────────────────────

export namespace almondnamespace
{
    using u8 = std::uint8_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    // ────────────────────────────────────────────────────────
    // ATLAS REGION
    // ────────────────────────────────────────────────────────

    struct AtlasRegion
    {
        float u1{}, v1{};
        float u2{}, v2{};
        u32   x{}, y{};
        u32   width{}, height{};

        [[nodiscard]] float uv_width()  const noexcept { return u2 - u1; }
        [[nodiscard]] float uv_height() const noexcept { return v2 - v1; }
    };

    // ────────────────────────────────────────────────────────
    // ATLAS ENTRY
    // ────────────────────────────────────────────────────────

    struct AtlasEntry
    {
        int                 index{ -1 };
        std::string         name;
        AtlasRegion         region;
        std::vector<u8>     pixels;
        u32                 texWidth{ 0 };
        u32                 texHeight{ 0 };

        AtlasEntry() = default;

        AtlasEntry(
            int idx,
            std::string name_,
            AtlasRegion region_,
            std::vector<u8> pixels_,
            u32 w,
            u32 h)
            : index(idx),
            name(std::move(name_)),
            region(region_),
            pixels(std::move(pixels_)),
            texWidth(w),
            texHeight(h)
        {
        }
    };

    // ────────────────────────────────────────────────────────
    // ATLAS CONFIG
    // ────────────────────────────────────────────────────────

    struct AtlasConfig
    {
        std::string name;
        u32 width{ 2048 };
        u32 height{ 2048 };
        bool generate_mipmaps{ false };
        int index{ 0 };
    };

    // ────────────────────────────────────────────────────────
    // TEXTURE ATLAS
    // ────────────────────────────────────────────────────────

    struct TextureAtlas
    {
        std::string name;
        int index{ -1 };
        u32 width{ 0 };
        u32 height{ 0 };
        bool has_mipmaps{ false };

        mutable u64 version{ 0 };
        mutable std::vector<u8> pixel_data;

        std::vector<AtlasEntry> entries;

        TextureAtlas() = default;

        TextureAtlas(std::string n, int w, int h)
            : name(std::move(n)), width(static_cast<u32>(w)), height(static_cast<u32>(h))
        {
        }

        TextureAtlas(const TextureAtlas&) = delete;
        TextureAtlas& operator=(const TextureAtlas&) = delete;
        TextureAtlas(TextureAtlas&&) = delete;
        TextureAtlas& operator=(TextureAtlas&&) = delete;

        bool init(const AtlasConfig& config)
        {
            name = config.name;
            index = config.index;
            width = config.width;
            height = config.height;
            has_mipmaps = config.generate_mipmaps;

            pixel_data.clear();
            pixel_data.resize(
                static_cast<std::size_t>(width) * height * 4, 0
            );

            occupancy.clear();
            occupancy.assign(height, std::vector<bool>(width, false));

            {
                std::unique_lock lock(entriesMutex);
                entries.clear();
                lookup.clear();
            }

            version = 0;
            return true;
        }

        [[nodiscard]] size_t entry_count() const noexcept
        {
            std::shared_lock lock(entriesMutex);
            return entries.size();
        }

        [[nodiscard]] bool try_get_entry_info(
            int index,
            AtlasRegion& outRegion,
            std::string* outName = nullptr) const
        {
            std::shared_lock lock(entriesMutex);

            if (index < 0 || static_cast<size_t>(index) >= entries.size())
                return false;

            const auto& entry = entries[static_cast<size_t>(index)];
            if (entry.region.width == 0 || entry.region.height == 0)
                return false;

            outRegion = entry.region;
            if (outName)
                *outName = entry.name;

            return true;
        }

        static std::unique_ptr<TextureAtlas> create(const AtlasConfig& config)
        {
            auto atlas = std::make_unique<TextureAtlas>();
            atlas->name = config.name;
            atlas->index = config.index;
            atlas->width = config.width;
            atlas->height = config.height;
            atlas->has_mipmaps = config.generate_mipmaps;
            atlas->pixel_data.resize(
                static_cast<size_t>(atlas->width) * atlas->height * 4, 0);
            atlas->occupancy.assign(
                atlas->height, std::vector<bool>(atlas->width, false));

            return atlas;
        }

        [[nodiscard]] int get_index() const noexcept { return index; }

        std::optional<AtlasEntry> add_entry(const std::string& id, const Texture& tex);
        std::optional<AtlasEntry> add_slice_entry(const std::string& id, int x, int y, int w, int h);
        std::optional<AtlasRegion> get_region(const std::string& id) const;
        void rebuild_pixels() const;

    private:
        mutable std::shared_mutex entriesMutex;
        std::unordered_map<std::string, AtlasRegion> lookup;
        std::vector<std::vector<bool>> occupancy;

        std::optional<std::pair<u32, u32>> try_pack(u32 w, u32 h);
        bool can_place(u32 x, u32 y, u32 w, u32 h) const;
        void mark_used(u32 x, u32 y, u32 w, u32 h);
    };
}
