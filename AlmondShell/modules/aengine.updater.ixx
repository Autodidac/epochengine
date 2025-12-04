module;

#include "aupdatesystem.hpp"

export module aengine.updater;

export namespace almondnamespace::updater
{
    export using ::almondnamespace::updater::UpdateChannel;
    export using ::almondnamespace::updater::UpdateCommandResult;
    export using ::almondnamespace::updater::run_update_command;
}
