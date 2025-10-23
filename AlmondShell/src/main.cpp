#include "ascriptingsystem.hpp"
#include "ataskscheduler.hpp"
#include "aupdatesystem.hpp"
#include "almondshell.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

// configuration overrides 
namespace urls {
    const std::string github_base = "https://github.com/"; // base github url
    const std::string github_raw_base = "https://raw.githubusercontent.com/"; // raw base github url, for source downloads

    const std::string owner = "Autodidac/"; // github project developer username for url 
    const std::string repo = "Cpp_Ultimate_Project_Updater"; // whatever your github project name is
    const std::string branch = "main/"; // incase you need a different branch than githubs default branch main 

    // It's now using this internal file to fetch update versions internally without version.txt file that can be modified
    const std::string version_url = github_raw_base + owner + repo + "/" + branch + "/include/config.hpp";
    //const std::string source_url = github_base + owner + repo + "/archive/refs/heads/main.zip";
    const std::string binary_url = github_base + owner + repo + "/releases/latest/download/updater.exe";
}

int main(int argc, char* argv[]) {
    std::vector<std::string_view> arguments;
    arguments.reserve(static_cast<std::size_t>(argc));
    for (int index = 0; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }

    const almondnamespace::updater::UpdateChannel channel{
        .version_url = urls::version_url,
        .binary_url = urls::binary_url,
    };

    const auto bootstrap_result = almondnamespace::updater::bootstrap_from_command(channel, arguments);
    if (bootstrap_result.should_exit) {
        return bootstrap_result.exit_code;
    }

    // Lets Begin
    TaskScheduler scheduler;

    std::cout << "[Engine] Starting up...\n";

    std::string scriptName = "editor_launcher";

    if (!almond::load_or_reload_script(scriptName, scheduler)) {
        std::cerr << "[Engine] Initial script load failed.\n";
    }

    auto lastCheck = std::filesystem::last_write_time("src/scripts/" + scriptName + ".ascript.cpp");
    auto start = std::chrono::steady_clock::now();

    while (true) {
        auto now = std::filesystem::last_write_time("src/scripts/" + scriptName + ".ascript.cpp");
        if (now != lastCheck) {
            std::cout << "\n[Engine] Detected change in script source, recompiling...\n";
            almond::load_or_reload_script(scriptName, scheduler);
            lastCheck = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count() > 10) break;
    }

    std::cout << "[Engine] Session ended.\n";
    return 0;
}