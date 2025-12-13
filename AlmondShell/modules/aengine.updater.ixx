module;

export module aengine.updater;

import aupdatesystem; // Primary updater implementation module

export namespace almondnamespace::updater
{
    export using ::almondnamespace::updater::UpdateChannel;
    export using ::almondnamespace::updater::UpdateCommandResult;
    // Ensure the two-parameter updater entry point is visible to importers
    export using ::almondnamespace::updater::run_update_command;
}
