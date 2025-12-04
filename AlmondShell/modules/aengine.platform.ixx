module;

export module aengine:platform;

import std;

import "aplatform.hpp";
import "aengineconfig.hpp";
import "aengine.hpp";
import :cli;
import "aupdateconfig.hpp";
import "aupdatesystem.hpp";
import "aplatformpump.hpp";
import "aguimenu.hpp";
import "aapplicationmodule.hpp";
import "aenduserapplication.hpp";
import "awindowdata.hpp";
import "acontext.hpp";
import "acontextmultiplexer.hpp";
import "acontextwindow.hpp";
import "ainput.hpp";
import "aversion.hpp";

export namespace almondnamespace
{
    namespace core
    {
        using ::almondnamespace::core::Context;
        using ::almondnamespace::core::WindowData;
    }

    namespace menu
    {
        using ::almondnamespace::menu::Menu;
        using ::almondnamespace::menu::MenuItem;
    }

    namespace updater
    {
        using ::almondnamespace::updater::OWNER;
        using ::almondnamespace::updater::REPO;
    }

    namespace platform
    {
        using ::almondnamespace::platform::pump_events;
    }
}
