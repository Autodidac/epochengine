#pragma once

#include "apluginmanager.h"

#include <future>
#include <thread>

namespace almondnamespace::plugin {

    // AsyncPluginManager extends PluginManager to add async loading of plugins
    class AsyncPluginManager : public PluginManager {
    public:
        std::future<bool> LoadPluginAsync(const std::filesystem::path& path) {
            return std::async(std::launch::async, [this, path]() {
                return LoadPlugin(path);
                });
        }
    };

} // namespace almond::plugin
