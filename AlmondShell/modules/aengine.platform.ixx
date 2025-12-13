export module aengine.platform;

import aplatform;
import aengineconfig;
import aengine;
import aengine.cli;
import aengine.updater;
import aupdateconfig;

import aguimenu;
import aapplicationmodule;
import aenduserapplication;
import awindowdata;
import acontext;
import acontextmultiplexer;
import acontextwindow;
import ainput;
import aengine.version;
import aplatformpump;

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
