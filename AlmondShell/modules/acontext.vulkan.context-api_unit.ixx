module;

export module acontext.vulkan.context;

import :api;            // brings in declarations for vulkan_* funcs
import :shared_vk; // brings in vulkan_app() / Application

import <functional>;
import <stdexcept>;
import <utility>;
import <memory>;
import <iostream>;

namespace almondnamespace::vulkancontext
{
    // Small ones first so they're visible no matter what.
    int vulkan_get_width()
    {
        return vulkan_app().get_framebuffer_width();
    }

    int vulkan_get_height()
    {
        return vulkan_app().get_framebuffer_height();
    }

    bool vulkan_initialize(
        std::shared_ptr<core::Context> ctx,
        void* parentWindowOpaque,
        unsigned int w,
        unsigned int h,
        std::function<void(int, int)> onResize)
    {
        if (!ctx)
            throw std::runtime_error("[Vulkan] vulkan_initialize requires non-null Context");

        void* nativeWindow = parentWindowOpaque;

#if defined(_WIN32) && !defined(ALMOND_MAIN_HEADLESS)
        if (ctx->windowData && ctx->windowData->hwnd)
            nativeWindow = ctx->windowData->hwnd;
        else if (ctx->get_hwnd())
            nativeWindow = ctx->get_hwnd(); // <-- CALL IT
#endif

        auto& app = vulkan_app();

        app.set_framebuffer_size(static_cast<int>(w), static_cast<int>(h));
        ctx->framebufferWidth = static_cast<int>(w);
        ctx->framebufferHeight = static_cast<int>(h);
        app.set_context(ctx, nativeWindow);

        ctx->get_width  = &vulkan_get_width;
        ctx->get_height = &vulkan_get_height;

        ctx->onResize = [&app, ctx, resize = std::move(onResize)](int nw, int nh) mutable
        {
            app.set_framebuffer_size(nw, nh);
            ctx->framebufferWidth = nw;
            ctx->framebufferHeight = nh;
            if (resize) resize(nw, nh);
        };

        app.initWindow();
        app.initVulkan();

        //vulkan_initialize
		std::cout << "[Vulkan] Initialized successfully.\n";

        return true;
    }

    bool vulkan_process(std::shared_ptr<core::Context> ctx, core::CommandQueue& queue)
    {
        return vulkan_app().process(ctx, queue);
    }

    void vulkan_present()
    {
        // no-op; presentation happens in Application flow
    }

    void vulkan_cleanup(std::shared_ptr<core::Context> ctx)
    {
        (void)ctx;
        vulkan_app().cleanup();
    }
}
