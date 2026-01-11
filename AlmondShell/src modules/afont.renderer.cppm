module;

#include <utility>

export module afont.renderer;

import afont.renderer;

namespace almondnamespace::font
{
    almondnamespace::font::FontRenderer::FontRenderer(logger::Logger* log)
        : logger_(log)
    {
    }

    bool FontRenderer::load_font(const std::string& name,
        const std::string& path)
    {
        if (loaded_fonts_.contains(name))
            return false;

        loaded_fonts_.emplace(
            name,
            FontAsset{ name, path }
        );

        return true;
    }

    bool FontRenderer::render_text_to_atlas(
        const std::string& font_name,
        const std::string&,
        TextureAtlas&,
        int,
        int)
    {
        return loaded_fonts_.contains(font_name);
    }

    void FontRenderer::unload_font(const std::string& name)
    {
        loaded_fonts_.erase(name);
    }
}
