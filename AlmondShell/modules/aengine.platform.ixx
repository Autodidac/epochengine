module;

export module aengine.platform;

import aengine.context;
import aengine.context.window;
import aplatformpump;
//import aengine.gui.menu;

export namespace almondnamespace
{
    namespace core
    {
        using almondnamespace::core::Context;
        using almondnamespace::contextwindow::WindowData;
    }

    namespace menu
    {
       // using almondnamespace::menu::MenuOverlay;
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

//export bool almondnamespace::platform::pump_events()
//{
//    return pump_events_impl();
//}
