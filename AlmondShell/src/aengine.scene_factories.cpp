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

#include "..\include\aengine.scene_factories.hpp"

import <memory>;

import asnakelike;
import atetrislike;
import apacmanlike;
import afroggerlike;
import asokobanlike;
import amatch3like;

import aslidingpuzzlelike;
import aminesweeperlike;
import a2048like;

import asandsim;
import acellularsim;

namespace almondnamespace::core::engine
{
    std::unique_ptr<almondnamespace::scene::Scene> CreateSnakeScene()
    {
        return std::make_unique<almondnamespace::snakelike::SnakeLikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreateTetrisScene()
    {
        return std::make_unique<almondnamespace::tetrislike::TetrisLikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreatePacmanScene()
    {
        return std::make_unique<almondnamespace::pacmanlike::PacmanLikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreateFroggerScene()
    {
        return std::make_unique<almondnamespace::froggerlike::FroggerLikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreateSokobanScene()
    {
        return std::make_unique<almondnamespace::sokobanlike::SokobanLikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreateMatch3Scene()
    {
        return std::make_unique<almondnamespace::match3like::Match3LikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreateSlidingPuzzleScene()
    {
        return std::make_unique<almondnamespace::slidinglike::SlidingPuzzleLikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreateMinesweeperScene()
    {
        return std::make_unique<almondnamespace::minesweeperlike::MinesweeperLikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> Create2048Scene()
    {
        return std::make_unique<almondnamespace::a2048like::A2048LikeScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreateSandSimScene()
    {
        return std::make_unique<almondnamespace::sandsim::SandSimScene>();
    }

    std::unique_ptr<almondnamespace::scene::Scene> CreateCellularSimScene()
    {
        return std::make_unique<almondnamespace::cellularsim::CellularSimScene>();
    }
}
