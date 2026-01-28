//// acontext.vulkan.platform.context.cpp
//
// This file MUST be a module implementation unit for `acontext.vulkan.context`
// because it defines `almondnamespace::vulkancontext::Application` methods.
//
// It also MUST be the single TU that provides:
//  - STB_IMAGE_IMPLEMENTATION
//  - VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
//
// If you keep vulkan.hpp in the module interface BMI, the safest way to guarantee
// the dispatch storage exists is to include vulkan.hpp textually here (with the
// same config macros) before `module acontext.vulkan.context;`.
//
// -----------------------------------------------------------------------------
//
// NOTE: No `import acontext.vulkan.context;` here. This file *is* that module.
//

module;

// ---- One-TU-only implementations / storage ---------------------------------
#ifndef STB_IMAGE_IMPLEMENTATION
#   define STB_IMAGE_IMPLEMENTATION
#endif

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#   define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#ifndef VULKAN_HPP_NO_EXCEPTIONS
#   define VULKAN_HPP_NO_EXCEPTIONS
#endif

// Define the dynamic dispatcher storage exactly once in the whole program:
#define VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <include/aengine.config.hpp>

#if defined(_WIN32)
#   ifndef VK_USE_PLATFORM_WIN32_KHR
#       define VK_USE_PLATFORM_WIN32_KHR
#   endif
#endif

#if defined(ALMOND_VULKAN_STANDALONE)
#   ifndef GLFW_INCLUDE_VULKAN
#       define GLFW_INCLUDE_VULKAN
#   endif
#   include <GLFW/glfw3.h>
#else
struct GLFWwindow;
#endif

#include <vulkan/vulkan.hpp>

// STB must be included after its implementation define:
#include "src/stb/stb_image.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// Now enter the named module (implementation unit)
// -----------------------------------------------------------------------------
module acontext.vulkan.context;

import :window;
import :instance;
import :device;
import :swapchain;
import :shader_pipeline;
import :depth;
import :memory;
import :texture;
import :descriptor;
import :commands;

import aatlas.manager;
import aatlas.texture;

import aengine.context.commandqueue;
import aengine.core.context;
import aengine.input;

// -----------------------------------------------------------------------------
// Correct namespace for the exported type declared in the interface:
// export namespace almondnamespace::vulkancontext { export class Application ... }
// -----------------------------------------------------------------------------
namespace almondnamespace::vulkancontext
{
    void Application::run()
    {
        almondnamespace::core::CommandQueue queue;
        initWindow();
        initVulkan();

#ifdef _DEBUG
        std::cout << "Vertex struct size: " << sizeof(Vertex) << "\n";
        std::cout << "Position offset: " << offsetof(Vertex, pos) << "\n";
        std::cout << "Normal offset: " << offsetof(Vertex, normal) << "\n";
        std::cout << "TexCoord offset: " << offsetof(Vertex, texCoord) << "\n";
#endif

        while (process(nullptr, queue)) {}
        cleanup();
    }

    void Application::initVulkan()
    {
#ifdef _DEBUG
        std::cout << "initVulkan() called\n";
#endif
        createInstance();
        createSurface();

        if (enableValidationLayers)
        {
            // setupDebugMessenger(); // if you implement it
        }

        physicalDevice = pickPhysicalDevice();
        if (!physicalDevice)
            throw std::runtime_error("Failed to pick a physical device!");

        queueFamilyIndices = findQueueFamilies(physicalDevice);

        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createDepthResources();
        createFramebuffers();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void Application::initWindow()
    {
#if defined(ALMOND_VULKAN_STANDALONE)
        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(framebufferWidth, framebufferHeight, "Vulkan Cube", nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Failed to create GLFW window");

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetCursorPosCallback(window, mouseCallback);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#else
        window = nullptr;
        if (!nativeWindowHandle)
            throw std::runtime_error("Vulkan requires a native window handle.");
#endif

        // IMPORTANT: with VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE defined
        // in THIS TU, this init call is valid and links cleanly.
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

#ifdef _DEBUG
        std::cout << "Window configured\n";
#endif
    }

    void Application::framebufferResizeCallback(GLFWwindow* window_, int width, int height)
    {
#if defined(ALMOND_VULKAN_STANDALONE)
        auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window_));
        if (!app)
            return;

        int fbWidth = width;
        int fbHeight = height;
        if (fbWidth == 0 || fbHeight == 0)
            glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);

        app->set_framebuffer_size(fbWidth, fbHeight);
#else
        (void)window_;
        (void)width;
        (void)height;
#endif
    }

    bool Application::process(std::shared_ptr<almondnamespace::core::Context> ctx,
        almondnamespace::core::CommandQueue& queue)
    {
        if (!device)
            return false;

        static auto lastTime = std::chrono::high_resolution_clock::now();
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

#if defined(ALMOND_VULKAN_STANDALONE)
        if (window)
        {
            glfwPollEvents();
            if (glfwWindowShouldClose(window))
                return false;
        }
#else
        if (ctx && ctx->get_mouse_position)
        {
            int mouseX = 0;
            int mouseY = 0;
            ctx->get_mouse_position(mouseX, mouseY);
            processMouseInput(static_cast<double>(mouseX), static_cast<double>(mouseY));
        }
#endif

        updateCamera(deltaTime);

        queue.drain();
        drawFrame();
        return true;
    }

    void Application::set_context(std::shared_ptr<almondnamespace::core::Context> ctx, void* nativeWindow)
    {
        context = std::move(ctx);
        nativeWindowHandle = nativeWindow;
    }

    void Application::set_framebuffer_size(int width, int height)
    {
        framebufferWidth = (std::max)(1, width);
        framebufferHeight = (std::max)(1, height);
        framebufferResized = true;
    }

    int Application::get_framebuffer_width() const noexcept
    {
        return (std::max)(1, framebufferWidth);
    }

    int Application::get_framebuffer_height() const noexcept
    {
        return (std::max)(1, framebufferHeight);
    }

    void Application::cleanup()
    {
        if (device)
            (void)device->waitIdle();

        imageAvailableSemaphores.clear();
        renderFinishedSemaphores.clear();
        inFlightFences.clear();

        if (!commandBuffers.empty() && commandPool && device)
        {
            device->resetCommandPool(*commandPool);
            commandBuffers.clear();
        }

        cleanupSwapChain();

        framebuffers.clear();

        descriptorPool.reset();

        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();

        indexBuffer.reset();
        indexBufferMemory.reset();
        vertexBuffer.reset();
        vertexBufferMemory.reset();

        textureSampler.reset();
        textureImageView.reset();
        textureImage.reset();
        textureImageMemory.reset();

        pipelineLayout.reset();
        graphicsPipeline.reset();
        descriptorSetLayout.reset();
        renderPass.reset();

        depthImage.reset();
        depthImageMemory.reset();
        depthImageView.reset();

        commandPool.reset();

        device.reset();

        // surface is a UniqueSurfaceKHR; do not manually destroy it.
        surface.reset();

        if (enableValidationLayers && debugMessenger && instance)
        {
            instance->destroyDebugUtilsMessengerEXT(debugMessenger, nullptr);
            debugMessenger = VK_NULL_HANDLE;
        }

        instance.reset();

#if defined(ALMOND_VULKAN_STANDALONE)
        if (window)
        {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
#endif
    }

    void Application::createVertexBuffer()
    {
        const vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        auto staging = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* mapped = nullptr;
        (void)device->mapMemory(*staging.second, 0, bufferSize, {}, &mapped);
        std::memcpy(mapped, vertices.data(), static_cast<std::size_t>(bufferSize));
        device->unmapMemory(*staging.second);

        auto vb = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        copyBuffer(*staging.first, *vb.first, bufferSize);

        staging.first.reset();
        staging.second.reset();

        vertexBuffer = std::move(vb.first);
        vertexBufferMemory = std::move(vb.second);
    }

    void Application::createIndexBuffer()
    {
        if (indices.empty())
            throw std::runtime_error("Error: No index data available!");

        const vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        auto staging = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        // Prefer the void** overload to avoid ResultValue plumbing under VULKAN_HPP_NO_EXCEPTIONS.
        void* mapped = nullptr;
        (void)device->mapMemory(*staging.second, 0, bufferSize, {}, &mapped);
        std::memcpy(mapped, indices.data(), static_cast<std::size_t>(bufferSize));
        device->unmapMemory(*staging.second);

        auto ib = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        copyBuffer(*staging.first, *ib.first, bufferSize);

        staging.first.reset();
        staging.second.reset();

        indexBuffer = std::move(ib.first);
        indexBufferMemory = std::move(ib.second);
    }
} // namespace almondnamespace::vulkancontext

// -----------------------------------------------------------------------------
// Engine-facing wrapper API (matches your exported declarations)
// -----------------------------------------------------------------------------
namespace almondnamespace::vulkancontext
{
    namespace
    {
        detail::Application s_app{};
    }

    bool vulkan_initialize(std::shared_ptr<core::Context> ctx,
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
            nativeWindow = ctx->get_hwnd();
#endif

        s_app.set_framebuffer_size(static_cast<int>(w), static_cast<int>(h));
        s_app.set_context(ctx, nativeWindow);

        ctx->get_width = vulkan_get_width;
        ctx->get_height = vulkan_get_height;

        ctx->is_key_held = [](almondnamespace::input::Key key) { return almondnamespace::input::is_key_held(key); };
        ctx->is_key_down = [](almondnamespace::input::Key key) { return almondnamespace::input::is_key_down(key); };
        ctx->is_mouse_button_held = [](almondnamespace::input::MouseButton b) { return almondnamespace::input::is_mouse_button_held(b); };
        ctx->is_mouse_button_down = [](almondnamespace::input::MouseButton b) { return almondnamespace::input::is_mouse_button_down(b); };

        ctx->onResize = [resize = std::move(onResize)](int newWidth, int newHeight) mutable
            {
                s_app.set_framebuffer_size(newWidth, newHeight);
                if (resize)
                    resize(newWidth, newHeight);
            };

        s_app.initWindow();
        s_app.initVulkan();

        almondnamespace::atlasmanager::register_backend_uploader(
            almondnamespace::core::ContextType::Vulkan,
            [](const almondnamespace::TextureAtlas& atlas)
            {
                almondnamespace::vulkantextures::ensure_uploaded(atlas);
            });

        return true;
    }

    bool vulkan_process(std::shared_ptr<core::Context> ctx, core::CommandQueue& queue)
    {
        return s_app.process(ctx, queue);
    }

    void vulkan_present()
    {
        // Present is handled by drawFrame() currently.
    }

    void vulkan_cleanup(std::shared_ptr<core::Context> ctx)
    {
        (void)ctx;

        almondnamespace::atlasmanager::unregister_backend_uploader(
            almondnamespace::core::ContextType::Vulkan);

        almondnamespace::vulkantextures::clear_gpu_atlases();

        s_app.cleanup();
    }

    int vulkan_get_width()
    {
        return s_app.get_framebuffer_width();
    }

    int vulkan_get_height()
    {
        return s_app.get_framebuffer_height();
    }
}
