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
// acommandline.hpp ─ AlmondShell command‑line utilities (header‑only)
// C++20, no .inl, functional free‑functions only.
#pragma once

#include "aversion.hpp"          // GetEngineName(), GetEngineVersion()

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace almondnamespace::core::cli {

    // ─── public, mutable knobs (inline globals) ────────────────────────
    inline int  window_width = DEFAULT_WINDOW_WIDTH;
    inline int  window_height = DEFAULT_WINDOW_HEIGHT;
    inline int  menu_columns = 4;
    inline std::filesystem::path exe_path;

    struct ParseResult
    {
        bool update_requested = false;
        bool force_update = false;
    };

    // ─── helpers ───────────────────────────────────────────────────────
    inline void print_engine_info() {
        std::cout << GetEngineName() << " v" << GetEngineVersion() << '\n';
    }

    // ─── parse() ───────────────────────────────────────────────────────
    inline ParseResult parse(int argc, char* argv[])
    {
        using namespace std::string_view_literals;
        ParseResult result{};
        if (argc < 1) {
            std::cerr << "No command-line arguments provided.\n";
            return result;
        }

        exe_path = argv[0];
        std::cout << "Commandline for " << exe_path.filename().string() << ":\n";
        for (int i = 1; i < argc; ++i) {
            std::string_view arg{ argv[i] };

            if (arg == "--help"sv || arg == "-h"sv) {
                std::cout <<
                    "  --help, -h            Show this help message\n"
                    "  --version, -v         Display the engine version\n"
                    "  --width  <value>      Set window width\n"
                    "  --height <value>      Set window height\n"
                    "  --menu-columns <n>    Cap the menu grid at n columns (default 4)\n"
                    "  --update, -u          Check for a newer AlmondShell build\n"
                    "  --force               Apply the available update immediately\n";
            }
            else if (arg == "--version"sv || arg == "-v"sv) {
                print_engine_info();
            }
            else if (arg == "--width"sv && i + 1 < argc) {
                window_width = std::stoi(argv[++i]);
            }
            else if (arg == "--height"sv && i + 1 < argc) {
                window_height = std::stoi(argv[++i]);
            }
            else if (arg == "--menu-columns"sv && i + 1 < argc) {
                menu_columns = std::max(1, std::stoi(argv[++i]));
            }
            else if (arg == "--update"sv || arg == "-u"sv) {
                result.update_requested = true;
            }
            else if (arg == "--force"sv) {
                result.force_update = true;
            }
            else {
                std::cerr << "Unknown arg: " << arg << '\n';
            }
        }
        std::cout << '\n';
        if (result.force_update && !result.update_requested) {
            std::cout << "[WARN] Ignoring --force without --update.\n";
        }
        return result;
    }

} // namespace almondnamespace::core::cli
