module;

//#include "aengine.hpp"          // DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT

export module aengine.core.commandline;

import <algorithm>;
import <cctype>;
import <filesystem>;
import <iostream>;
import <optional>;
import <string>;
import <string_view>;

//import aengine;
import aengine.version;

inline constexpr int DEFAULT_WINDOW_WIDTH = 1280;
inline constexpr int DEFAULT_WINDOW_HEIGHT = 720;

export namespace almondnamespace::core::cli 
{
    inline int  window_width = DEFAULT_WINDOW_WIDTH;
    inline int  window_height = DEFAULT_WINDOW_HEIGHT;
    inline bool window_width_overridden = false;
    inline bool window_height_overridden = false;
    inline int  menu_columns = 4;
    inline bool trace_menu_button0_rect = true;
    inline bool trace_raylib_design_metrics = false;
    inline bool run_menu_loop = false;
    inline std::filesystem::path exe_path;
    inline std::optional<std::string> backend_filter;

    struct ParseResult {
        bool update_requested = false;
        bool force_update = false;
        bool editor_requested = false;
    };

    inline void print_engine_info() {
        std::cout << almondnamespace::GetEngineName() << " v" << almondnamespace::GetEngineVersion() << '\n';
    }

    inline ParseResult parse(int argc, char* argv[]) {
        using namespace std::string_view_literals;
        ParseResult result{};
        trace_menu_button0_rect = false;
        trace_raylib_design_metrics = false;
        run_menu_loop = false;
        window_width_overridden = false;
        window_height_overridden = false;
        backend_filter.reset();
        if (argc < 1) {
            std::cerr << "No command-line arguments provided.\n";
            return result;
        }

        auto normalize_backend = [](std::string_view value) {
            std::string normalized{ value };
            std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            return normalized;
        };
        auto is_known_backend = [](std::string_view backend) {
            return backend == "opengl"sv
                || backend == "sdl"sv
                || backend == "sfml"sv
                || backend == "vulkan"sv
                || backend == "software"sv
                || backend == "raylib"sv
                || backend == "raylib_nogl"sv
                || backend == "all"sv;
        };

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
                    "  --trace-menu-button0  Log GUI bounds for menu button index 0\n"
                    "  --trace-raylib-design Log framebuffer vs design canvas dimensions\n"
                    "  --editor              Start the editor interface\n"
                    "  --menu                Start the menu + games loop\n"
                    "  --update, -u          Check for a newer AlmondShell build\n"
                    "  --force               Apply the available update immediately\n"
                    "  --backend <name>      Run a single backend (opengl|sdl|sfml|vulkan|software|raylib)\n";
            }
            else if (arg == "--version"sv || arg == "-v"sv) {
                print_engine_info();
            }
            else if (arg == "--width"sv && i + 1 < argc) {
                window_width = std::stoi(argv[++i]);
                window_width_overridden = true;
            }
            else if (arg == "--height"sv && i + 1 < argc) {
                window_height = std::stoi(argv[++i]);
                window_height_overridden = true;
            }
            else if (arg == "--menu-columns"sv && i + 1 < argc) {
                menu_columns = (std::max)(1, std::stoi(argv[++i]));
            }
            else if (arg == "--trace-menu-button0"sv) {
                trace_menu_button0_rect = true;
            }
            else if (arg == "--trace-raylib-design"sv) {
                trace_raylib_design_metrics = true;
            }
            else if (arg == "--update"sv || arg == "-u"sv) {
                result.update_requested = true;
            }
            else if (arg == "--editor"sv) {
                result.editor_requested = true;
            }
            else if (arg == "--menu"sv) {
                run_menu_loop = true;
            }
            else if (arg == "--force"sv) {
                result.force_update = true;
            }
            else if (arg == "--backend"sv && i + 1 < argc) {
                const auto normalized = normalize_backend(argv[++i]);
                if (!is_known_backend(normalized)) {
                    std::cerr << "Unknown backend: " << normalized << '\n';
                } else if (normalized == "all") {
                    backend_filter.reset();
                } else {
                    backend_filter = normalized;
                }
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
}
