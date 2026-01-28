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

//#include "pch.h"
//
//#include "aplatform.hpp"     // must be first
//#include "aengineconfig.hpp"
//
//#include "agui.hpp"

module aeditor;

import aengine.gui;
import aengine.core.context;
import autility.filewatch;
import <algorithm>;

namespace almondnamespace
{
    bool editor_run(const std::shared_ptr<core::Context>& ctx,
        gui::WidgetBounds* out_bounds)
    {
        constexpr gui::Vec2 window_size{ 520.0f, 360.0f };
        constexpr float base_margin = 24.0f;

        const float viewport_width = ctx ? static_cast<float>(ctx->get_width_safe()) : 0.0f;
        const float viewport_height = ctx ? static_cast<float>(ctx->get_height_safe()) : 0.0f;

        const float dynamic_margin_x = (viewport_width > 0.0f)
            ? (std::max)(base_margin, viewport_width * 0.04f)
            : base_margin;
        const float dynamic_margin_y = (viewport_height > 0.0f)
            ? (std::max)(base_margin, viewport_height * 0.04f)
            : base_margin;

        gui::Vec2 window_position{ dynamic_margin_x, dynamic_margin_y };
        if (viewport_width > 0.0f && viewport_height > 0.0f)
        {
            window_position.x = std::clamp(
                window_position.x,
                0.0f,
                (std::max)(0.0f, viewport_width - window_size.x));
            window_position.y = std::clamp(
                window_position.y,
                0.0f,
                (std::max)(0.0f, viewport_height - window_size.y));
        }

        if (out_bounds)
        {
            out_bounds->position = window_position;
            out_bounds->size = window_size;
        }

        gui::begin_window("Epoch Editor", window_position, window_size);

        gui::label("Editor UI is active.");
        //gui::spacer(8.0f);

        const bool run_clicked = gui::button("Run Game", { 180.0f, 0.0f });
        gui::label(run_clicked ? "Launching menu..." : "Click Run Game to choose a scene.");

        gui::end_window();

        return run_clicked;

        //static auto files = get_file_states("epoch_gui_editor/src/scripts");
        //scan_and_mark_changes(files);

        //for (auto& f : files)
        //{
        //    ui::label(f.path);
        //}

        //ui::end_window();
    }
}
