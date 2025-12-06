module;

#include "aversion.hpp"

export module aversion;

export namespace almondnamespace {
    export using ::almondnamespace::major;
    export using ::almondnamespace::minor;
    export using ::almondnamespace::revision;

    export using ::almondnamespace::kEngineName;

    export using ::almondnamespace::GetMajor;
    export using ::almondnamespace::GetMinor;
    export using ::almondnamespace::GetRevision;

    export using ::almondnamespace::GetEngineName;
    export using ::almondnamespace::GetEngineNameView;
    export using ::almondnamespace::GetEngineVersion;
    export using ::almondnamespace::GetEngineVersionString;
}

export namespace almondshell = almondnamespace;
