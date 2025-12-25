module; // global module fragment
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <iostream>

export module aengine.scripting.compiler;

import aengine.scripting.compiler;

namespace almondnamespace::compiler
{
    bool compile_script_to_dll(
        const std::filesystem::path& input,
        const std::filesystem::path& output
    )
    {
        std::vector<std::string> clangArgs{
            "clang++",
            "-std=c++20",
            "-shared",
            input.string(),
            "-o", output.string(),
            "-Iinclude",
            "-fno-rtti",
            "-fno-exceptions",
            "-O2"
        };

        std::string cmd;
        for (const auto& arg : clangArgs)
        {
            cmd += arg;
            cmd += ' ';
        }

        std::cout << "[compiler] running: " << cmd << '\n';

        const int result = std::system(cmd.c_str());
        if (result != 0)
        {
            std::cerr << "[compiler] clang++ failed with code: "
                      << result << '\n';
            return false;
        }

        return true;
    }
}
