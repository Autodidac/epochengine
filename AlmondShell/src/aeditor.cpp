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
import autility.filewatch;

namespace almondnamespace
{
    bool editor_run()
    {
        gui::begin_window("Epoch Editor", { 20.0f, 20.0f }, { 520.0f, 360.0f });

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
