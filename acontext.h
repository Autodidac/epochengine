#pragma once

#include "aplatform.h"
// leave this here, avoid auto sorting incorrect order, platform always comes first

#include <memory>

namespace almondnamespace {

    // Abstract Context interface (no change from before)
    struct Context {
        void (*initialize)();
        void (*destroy)();
        void (*process)();
    };

    using ContextPtr = std::shared_ptr<Context>;

    enum class ContextType {
        OpenGL,
        Vulkan,
        DirectX // Add more types as needed
    };

    ContextPtr create_context(ContextType type);

} // namespace almondnamespace
