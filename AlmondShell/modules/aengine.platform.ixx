module;

#define ALMOND_PLATFORM_PUMP_DECLARE_ONLY
#include "aplatformpump.hpp"
#undef ALMOND_PLATFORM_PUMP_DECLARE_ONLY

export module aengine:platform;

import "aplatform.hpp";
import "aengineconfig.hpp";
import "aengine.hpp";
import aengine.cli;
import aengine.updater;
import "aupdateconfig.hpp";

import "aguimenu.hpp";
import "aapplicationmodule.hpp";
import aenduserapplication;
import "awindowdata.hpp";
import "acontext.hpp";
import "acontextmultiplexer.hpp";
import "acontextwindow.hpp";
import "ainput.hpp";
import aversion;

export namespace almondnamespace
{
    namespace core
    {
        using ::almondnamespace::core::Context;
        using ::almondnamespace::core::WindowData;
    }

    namespace menu
    {
        using ::almondnamespace::menu::MenuOverlay;
    }

    namespace updater
    {
        //using ::almondnamespace::updater::OWNER;
       // using ::almondnamespace::updater::REPO;
    }

    namespace platform
    {
        export bool pump_events();
    }
}

export bool almondnamespace::platform::pump_events()
{
    return pump_events_impl();
}
