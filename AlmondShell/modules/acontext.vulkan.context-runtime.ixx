module;

#include <include/acontext.vulkan.hpp>

export module acontext.vulkan.context:runtime;

import :shared_context;

// Import any other partitions whose helpers you call from these method bodies.
// import :instance;
// import :device;
// import :swapchain;
// import :commands;
// import :renderer;
// etc.

namespace almondnamespace::vulkancontext
{
    // SINGLE definition lives here.
    // (Declaration is exported from :shared_context.)
    Application& vulkan_app()
    {
        static Application app{};
        return app;
    }

    // ---- Application method definitions ----

    void Application::initWindow()
    {
        // move your existing initWindow body here
    }

    void Application::initVulkan()
    {
        // move your existing initVulkan body here
    }

    bool Application::process(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        // move your existing process body here
        // return running state
        (void)ctx;
        (void)queue;
        return true;
    }

    void Application::cleanup()
    {
        // move your existing cleanup body here
    }

    void Application::set_framebuffer_size(int width, int height)
    {
        framebufferWidth = width;
        framebufferHeight = height;
        framebufferResized = true;
    }

    int Application::get_framebuffer_width() const noexcept { return framebufferWidth; }
    int Application::get_framebuffer_height() const noexcept { return framebufferHeight; }

    void Application::set_context(std::shared_ptr<almondnamespace::core::Context> ctx, void* nativeWindow)
    {
        context = ctx;
        nativeWindowHandle = nativeWindow;
    }
}
