// Crossplatform Utilities For Everyone

#pragma once

// Crossplatform Console Check 
#ifdef _WIN32

    #include "aframework.h"

    #include <consoleapi3.h>

    namespace almondnamespace::utilities
    {
        inline bool isConsoleApplication()
        {
            return GetConsoleWindow() != nullptr;
        }
    }

#else

    // other platform includes
    namespace almondnamespace::utilities
    {
        inline bool isConsoleApplication()
        {
            return isatty(fileno(stdout));
        }
    }

#endif