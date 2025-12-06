module;

#include "abuildsystem.hpp"
#include "aupdateconfig.hpp"

import std;

export module aupdatesystem;

export namespace almondnamespace
{
    namespace cmds
    {
        inline constexpr std::string_view kUpdateLong{ "--update" };
        inline constexpr std::string_view kUpdateShort{ "-u" };
        inline constexpr std::string_view kForceFlag{ "--force" };
        inline constexpr std::string_view kHelpLong{ "--help" };
        inline constexpr std::string_view kHelpShort{ "-h" };

        void cleanup_previous_update_artifacts();
    }

    namespace updater
    {
        void replace_binary_from_script(const std::string& new_binary);

        bool move_directory_contents(const std::filesystem::path& source,
                                     const std::filesystem::path& destination);

        void replace_binary(const std::string& new_binary);

        bool check_for_updates(const std::string& remote_config_url);

        bool install_from_binary(const std::string& binary_url);

        void install_from_source(const std::string& binary_url);

        void update_project(const std::string& source_url, const std::string& binary_url);

        struct UpdateChannel
        {
            std::string version_url;
            std::string binary_url;
        };

        struct UpdateCommandResult
        {
            bool update_available = false;
            bool update_performed = false;
            bool force_required = false;
            int exit_code = 0;
        };

        UpdateCommandResult run_update_command(const UpdateChannel& channel, bool force_install);
    }
}
