#include "aupdatesystem.hpp"

#include "aupdateconfig.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace almondnamespace::updater
{
    namespace
    {
        constexpr std::string_view kUpdateLong{"--update"};
        constexpr std::string_view kUpdateShort{"-u"};
        constexpr std::string_view kForceFlag{"--force"};
        constexpr std::string_view kHelpLong{"--help"};
        constexpr std::string_view kHelpShort{"-h"};

        bool matches_command(std::string_view argument, std::string_view option) noexcept
        {
            return argument == option;
        }

        bool contains_option(std::span<const std::string_view> arguments,
                              std::string_view option) noexcept
        {
            return std::any_of(arguments.begin(), arguments.end(),
                               [&](std::string_view value) { return matches_command(value, option); });
        }
    } // namespace

    void cleanup_previous_update_artifacts()
    {
        namespace fs = std::filesystem;

        std::vector<std::filesystem::path> cleanup_targets;
#ifdef LEAVE_NO_FILES_ALWAYS_REDOWNLOAD
#if defined(_WIN32)
        cleanup_targets.emplace_back(config::REPLACE_RUNNING_EXE_SCRIPT_NAME());
#else
        cleanup_targets.emplace_back("replace_and_restart.sh");
        cleanup_targets.emplace_back("replace_updater");
#endif
        cleanup_targets.emplace_back(config::REPO + "-main");
#endif

        for (const auto& target : cleanup_targets)
        {
            if (target.empty())
            {
                continue;
            }

            std::error_code ec;
            if (fs::is_directory(target, ec))
            {
                fs::remove_all(target, ec);
            }
            else
            {
                fs::remove(target, ec);
            }

            if (ec && ec != std::errc::no_such_file_or_directory)
            {
                std::cerr << "[WARN] Failed to remove artifact '" << target.string()
                          << "': " << ec.message() << '\n';
            }
        }
    }

    BootstrapResult bootstrap_from_command(const UpdateChannel& channel,
                                           std::span<const std::string_view> arguments)
    {
        cleanup_previous_update_artifacts();

        const auto begin = arguments.begin();
        const auto end = arguments.end();

        const bool update_requested = std::any_of(begin, end, [](std::string_view value) {
            return value == kUpdateLong || value == kUpdateShort;
        });

        const bool force_requested = contains_option(arguments, kForceFlag);
        const bool help_requested = contains_option(arguments, kHelpLong) || contains_option(arguments, kHelpShort);

        BootstrapResult result{};

        if (!update_requested)
        {
            if (check_for_updates(channel.version_url))
            {
                std::cout << "[INFO] New version available!\n";
                update_project(channel.version_url, channel.binary_url);
                result.update_performed = true;
            }
            else
            {
                std::cout << "[INFO] No updates available.\n";
            }

            return result;
        }

        result.should_exit = true;

        if (help_requested || !force_requested)
        {
            std::cout << "Usage: updater --update [--force]\n\n";
            std::cout << "Use --update --force to apply the latest release. Without --force the updater will"
                         " perform a version check only.\n";

            if (check_for_updates(channel.version_url))
            {
                std::cout << "[INFO] Update available but not applied. Re-run with --force to continue.\n";
            }
            else
            {
                std::cout << "[INFO] No updates available.\n";
            }

            return result;
        }

        if (check_for_updates(channel.version_url))
        {
            std::cout << "[INFO] New version available!\n";
            update_project(channel.version_url, channel.binary_url);
            result.update_performed = true;
        }
        else
        {
            std::cout << "[INFO] No updates available.\n";
        }

        return result;
    }
} // namespace almondnamespace::updater
