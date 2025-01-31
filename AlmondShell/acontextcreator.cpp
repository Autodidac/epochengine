
#include "pch.h"

#include "aopenglcontext.h"
#include "avulkanglfwcontext.h"

namespace almondnamespace::opengl {}
namespace almondnamespace::vulkan {}
namespace almondnamespace::directx {}

namespace almondnamespace {

    // Factory function to create a specific context based on ContextType
    ContextPtr create_context(ContextType type) {
        switch (type) {

        case ContextType::OpenGL:
            return std::make_shared<Context>(Context{

#ifdef ALMOND_USING_OPENGL

                almondnamespace::opengl::initialize,
                almondnamespace::opengl::destroy,
                almondnamespace::opengl::process

#endif

                });

        case ContextType::Vulkan:
            return std::make_shared<Context>(Context{

#ifdef ALMOND_USING_VULKAN

                almondnamespace::vulkan::initialize,
                almondnamespace::vulkan::destroy,
                almondnamespace::vulkan::process
#endif

                });

        case ContextType::DirectX:
            return std::make_shared<Context>(Context{

#ifdef ALMOND_USING_DIRECTX

                almondnamespace::directx::initialize,
                almondnamespace::directx::destroy,
                almondnamespace::directx::process

#endif

                });
        default:
            throw std::invalid_argument("Unsupported context type");
        }
    }
} // namespace almondnamespace

