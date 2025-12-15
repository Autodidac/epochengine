/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 *                                                            *
 *   This file is part of the Almond Project.                 *
 *   AlmondShell - Modular C++ Framework                      *
 *                                                            *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
 *                                                            *
 **************************************************************/
 // aengine.context.ixx

export module aengine.context;

import <map>;
import <memory>;
import <shared_mutex>;

export import aengine.context.type;
export import aengine.context.state;
export import aengine.context.commandqueue;

export namespace almondnamespace::core
{
    // Globals: exported declarations, single definition lives in aengine.context.cppm.
    extern std::map<ContextType, BackendState> g_backends;
    extern std::shared_mutex                   g_backendsMutex;

    // API
    void AddContextForBackend(ContextType type, std::shared_ptr<Context> context);
    void InitializeAllContexts();
    bool ProcessAllContexts();
    std::shared_ptr<Context> CloneContext(const Context& prototype);
}
