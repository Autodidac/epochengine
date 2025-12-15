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
export module aecs.components;

import <string>;
import <utility>;
import <vector>;

import aengine.core.logger;
import aengine.core.time;

export namespace almondnamespace::ecs
{
    using namespace almondnamespace::logger;
	using namespace almondnamespace::timing;
    // ─── Position ─────────────────────────────────────────────────────────
    struct Position { float x{ 0 }, y{ 0 }; };

    // ─── Velocity ─────────────────────────────────────────────────────────
    struct Velocity { float dx{ 0 }, dy{ 0 }; };

    // ─── History ──────────────────────────────────────────────────────────
    // Each entity that needs rewind support keeps its past states here
    struct History { std::vector<std::pair<float, float>> states; };

    // ─── LoggerComponent ─────────────────────────────────────────────────
    // Stores per-entity logging preferences (file, level, clock pointer)
    struct LoggerComponent {
        std::string       file;
        LogLevel          level{ LogLevel::INFO };
        Timer* clock{};
    };
}
