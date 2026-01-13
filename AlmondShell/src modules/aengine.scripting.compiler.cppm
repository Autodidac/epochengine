// aengine.scripting.compiler.cppm / .ixx
module; // global module fragment (allowed to use headers here)

#include <cstdlib>   // std::system
#include <iostream>  // std::cout / std::cerr

export module aengine.scripting.compiler;

// Prefer standard library module imports in the module purview.
import <filesystem>;
import <string>;
import <string_view>;
import <vector>;

export namespace almondnamespace::compiler
{
    // Builds a single TU into a shared library (DLL/.so).
    // Returns true on success (exit code == 0).
    export inline bool compile_script_to_dll(
        const std::filesystem::path& input,
        const std::filesystem::path& output
    )
    {
        std::vector<std::string> args;
        args.reserve(32);

#if defined(_WIN32)
        // If you're using MSVC-style link, you likely want clang-cl instead.
        // But keep clang++ since that's what your code already used.
        args.emplace_back("clang++");
#else
        args.emplace_back("clang++");
        args.emplace_back("-fPIC");
#endif

        args.emplace_back("-std=c++23");
        args.emplace_back("-shared");

        args.emplace_back(input.string());

        args.emplace_back("-o");
        args.emplace_back(output.string());

        // TODO: adjust include roots for your layout (keep as-is for now).
        args.emplace_back("-Iinclude");

        // Match your engine defaults (no RTTI/exceptions) as requested.
        args.emplace_back("-fno-rtti");
        args.emplace_back("-fno-exceptions");

        args.emplace_back("-O2");

        // Build command line (simple quoting for spaces).
        std::string cmd;
        cmd.reserve(4096);

        for (const auto& a : args)
        {
            const bool needs_quotes = (a.find(' ') != std::string::npos) || (a.find('\t') != std::string::npos);
            if (needs_quotes)
            {
                cmd.push_back('"');
                cmd += a;
                cmd.push_back('"');
            }
            else
            {
                cmd += a;
            }
            cmd.push_back(' ');
        }

        std::cout << "[compiler] running: " << cmd << '\n';

        const int result = std::system(cmd.c_str());
        if (result != 0)
        {
            std::cerr << "[compiler] clang failed with code: " << result << '\n';
            return false;
        }

        return true;
    }
} // namespace almondnamespace::compiler
