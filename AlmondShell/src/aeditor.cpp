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
    void editor_run()
    {
        gui::begin_window("Epoch Editor", { 20.0f, 20.0f }, { 480.0f, 320.0f });
        gui::label("Editor UI is active.");
        gui::label("Select Run Game to launch a scene.");
        gui::end_window();

        //static auto files = get_file_states("epoch_gui_editor/src/scripts");
        //scan_and_mark_changes(files);

        //for (auto& f : files)
        //{
        //    ui::label(f.path);
        //}

        //ui::end_window();
    }
}
