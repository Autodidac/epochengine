#pragma once

namespace almondnamespace::plugin {

    class IPlugin {
    public:
        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual ~IPlugin() = default;
    };

} // namespace almondnamespace::plugin
