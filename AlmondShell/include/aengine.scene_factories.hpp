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

#pragma once

#include <memory>

namespace almondnamespace::scene
{
    class Scene;
}

namespace almondnamespace::core::engine
{
    std::unique_ptr<almondnamespace::scene::Scene> CreateSnakeScene();
    std::unique_ptr<almondnamespace::scene::Scene> CreateTetrisScene();
    std::unique_ptr<almondnamespace::scene::Scene> CreatePacmanScene();
    std::unique_ptr<almondnamespace::scene::Scene> CreateFroggerScene();
    std::unique_ptr<almondnamespace::scene::Scene> CreateSokobanScene();
    std::unique_ptr<almondnamespace::scene::Scene> CreateMatch3Scene();
    std::unique_ptr<almondnamespace::scene::Scene> CreateSlidingPuzzleScene();
    std::unique_ptr<almondnamespace::scene::Scene> CreateMinesweeperScene();
    std::unique_ptr<almondnamespace::scene::Scene> Create2048Scene();
    std::unique_ptr<almondnamespace::scene::Scene> CreateSandSimScene();
    std::unique_ptr<almondnamespace::scene::Scene> CreateCellularSimScene();
}
