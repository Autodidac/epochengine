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
 *   See LICENSE file for full terms.                         *
 *                                                            *
 **************************************************************/
//afontrenderer.hpp

//#include "aplatform.hpp"
//#include "acontext.hpp"
//#include "aatlastexture.hpp"
//#include "aatlasmanager.hpp"
//#include "alogger.hpp"
//
//#include <string>
//#include <unordered_map>
//#include <cstdint>
//#include <memory>
module;

export module afont.renderer;

import <string>;
import <unordered_map>;
import <cstdint>;

// import the modules that define these types
import aengine.core.logger;
import aatlas.texture;

export namespace almondnamespace::font
{
    struct FontAsset
    {
        std::string name;
        std::string path;
    };

   export class FontRenderer
    {
    public:
        explicit FontRenderer(logger::Logger* log = nullptr);

        bool load_font(const std::string& name,
            const std::string& path);

        bool render_text_to_atlas(const std::string& font_name,
            const std::string& text,
            TextureAtlas& atlas,
            int x,
            int y);

        void unload_font(const std::string& name);

    private:
        logger::Logger* logger_{};
        std::unordered_map<std::string, FontAsset> loaded_fonts_;
    };
}
