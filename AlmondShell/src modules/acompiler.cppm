// src modules/acompiler.cppm
module;

#include "pch.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

export module autility.compiler;

namespace almondnamespace::compiler
{
    // Compile a single C++ script file into a DLL/shared library using clang++.
    // Returns true on success.
    export bool compile_script_to_dll(const std::filesystem::path& input,
        const std::filesystem::path& output)
    {
        // NOTE: you likely want toolchain selection per-platform;
        // this is intentionally simple and matches your earlier approach.
        std::vector<std::string> clangArgs = {
            "clang++",
            "-std=c++23",
            "-shared",
            input.string(),
            "-o", output.string(),
            "-Iinclude",
            "-O2",
            "-fno-rtti",
            "-fno-exceptions"
        };

        std::string cmd;
        cmd.reserve(1024);
        for (const auto& arg : clangArgs)
        {
            cmd += arg;
            cmd += ' ';
        }

        std::cout << "[compiler] running: " << cmd << "\n";
        const int result = std::system(cmd.c_str());
        if (result != 0)
        {
            std::cerr << "[compiler] clang++ failed with code: " << result << "\n";
            return false;
        }
        return true;
    }
}
