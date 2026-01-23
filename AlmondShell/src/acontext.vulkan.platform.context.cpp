// acontext.vulkan.platform.context.cpp

// Define STB_IMAGE_IMPLEMENTATION in only one translation unit. Static STB implementation
#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <stdexcept>
#include <chrono>
//#include "../include/vulkan_engine.hpp"

// Define the dispatch loader storage only here.
#define VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

//// Include subsystem implementations.
//#include "../include/vulkan_window.hpp"
//#include "../include/vulkan_instance.hpp"
//#include "../include/vulkan_device.hpp"
//#include "../include/vulkan_swapchain.hpp"
//#include "../include/vulkan_pipeline.hpp"
//#include "../include/vulkan_depth.hpp"
//#include "../include/vulkan_memory.hpp"
//#include "../include/vulkan_texture.hpp"
//#include "../include/vulkan_descriptor.hpp"
//#include "../include/vulkan_commands.hpp"

import acontext.vulkan.context;

import acontext.vulkan.window;
import acontext.vulkan.instance;
import acontext.vulkan.device;
import acontext.vulkan.swapchain;
import acontext.vulkan.pipeline;
import acontext.vulkan.depth;
import acontext.vulkan.memory;
import acontext.vulkan.texture;
import acontext.vulkan.descriptor;
import acontext.vulkan.commands;


namespace VulkanCube {

    // --------------------------------------------------------------------------
    // initVulkan() calls all necessary initialization functions.
    // --------------------------------------------------------------------------
    void Application::initVulkan() {
        std::cout << "initVulkan() called" << std::endl;
        createInstance();         // Defined in engine.cpp (see below)
        createSurface();

        if (enableValidationLayers) {
            // Optionally set up the debug messenger here.
            // setupDebugMessenger();
        }

        physicalDevice = pickPhysicalDevice();
        if (!physicalDevice) {
            throw std::runtime_error("Failed to pick a physical device!");
        }

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

        //  std::cout << "Indices count: " << indices.size() << std::endl;

        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects(); // Now sync objects (semaphores, fences) are created.
    }

    // --------------------------------------------------------------------------
    // initWindow() creates a GLFW window.
    // --------------------------------------------------------------------------
    void Application::initWindow() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(800, 600, "Vulkan Cube", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window");
        }
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetCursorPosCallback(window, mouseCallback);

        // Lock the mouse for camera control
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Initialize global Vulkan function pointers.
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        std::cout << "Window created" << std::endl;
    }

    // --------------------------------------------------------------------------
    // Callback: marks framebufferResized true when the window is resized.
    // --------------------------------------------------------------------------
    void Application::framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
        auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app) {
            app->framebufferResized = true;
        }
    }

    // --------------------------------------------------------------------------
    // mainLoop() processes window events, updates the camera, and draws frames.
    // --------------------------------------------------------------------------
    void Application::mainLoop() {
        auto lastTime = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            updateCamera(deltaTime);
            drawFrame();
        }
        (void)device->waitIdle();
    }

    // --------------------------------------------------------------------------
    // cleanup() releases all Vulkan and GLFW resources.
    // --------------------------------------------------------------------------
    void Application::cleanup() {
        if (device) {
            std::cout << "Waiting for device to finish all GPU work..." << std::endl;
            (void)device->waitIdle();
        }

        /*** 🔴 STEP 1: CLEANUP FRAME RESOURCES FIRST 🔴 ***/
        std::cout << "Destroying frame synchronization objects..." << std::endl;
        imageAvailableSemaphores.clear();
        renderFinishedSemaphores.clear();
        inFlightFences.clear();

        /*** 🔴 STEP 2: FREE COMMAND BUFFERS BEFORE DESTROYING COMMAND POOL 🔴 ***/
        std::cout << "Freeing command buffers..." << std::endl;
        if (!commandBuffers.empty() && commandPool) {
            device->resetCommandPool(*commandPool);
            commandBuffers.clear();
        }

        /*** 🔴 STEP 3: DESTROY SWAP CHAIN BEFORE DEVICE RESOURCES 🔴 ***/
        std::cout << "Destroying swap chain..." << std::endl;
        cleanupSwapChain();  // ✅ Swap chain must be destroyed BEFORE the device

        /*** 🔴 STEP 4: CLEANUP DEVICE RESOURCES 🔴 ***/
        std::cout << "Destroying framebuffers..." << std::endl;
        framebuffers.clear();

        std::cout << "Destroying descriptor pool..." << std::endl;
        descriptorPool.reset();

        std::cout << "Destroying uniform buffers..." << std::endl;
        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();

        std::cout << "Destroying index and vertex buffers..." << std::endl;
        indexBuffer.reset();
        indexBufferMemory.reset();
        vertexBuffer.reset();
        vertexBufferMemory.reset();

        std::cout << "Destroying textures..." << std::endl;
        textureSampler.reset();
        textureImageView.reset();
        textureImage.reset();
        textureImageMemory.reset();

        /*** 🔴 STEP 5: CLEANUP PIPELINE & DEPTH BUFFER 🔴 ***/
        std::cout << "Destroying pipeline and render pass..." << std::endl;
        pipelineLayout.reset();
        graphicsPipeline.reset();
        descriptorSetLayout.reset();
        renderPass.reset();

        std::cout << "Destroying depth buffer..." << std::endl;
        depthImage.reset();
        depthImageMemory.reset();
        depthImageView.reset();

        /*** 🔴 STEP 6: DESTROY COMMAND POOL LAST 🔴 ***/
        std::cout << "Destroying command pool..." << std::endl;
        commandPool.reset();

        /*** 🔴 STEP 7: DESTROY VULKAN DEVICE 🔴 ***/
        std::cout << "Destroying logical device..." << std::endl;
        if (device) {
            device.reset();
        }
        else {
            std::cerr << "Error: Device is already NULL before reset!" << std::endl;
        }

        /*** 🔴 STEP 8: DESTROY SURFACE BEFORE INSTANCE 🔴 ***/
        std::cout << "Destroying Vulkan surface..." << std::endl;
        if (instance && surface) {
            instance->destroySurfaceKHR(*surface);
            surface.reset();
        }

        /*** 🔴 STEP 9: DESTROY DEBUG MESSENGER BEFORE INSTANCE 🔴 ***/
        if (enableValidationLayers && debugMessenger) {
            std::cout << "Destroying Vulkan Debug Messenger..." << std::endl;
            instance->destroyDebugUtilsMessengerEXT(debugMessenger, nullptr);
            debugMessenger = nullptr;
        }

        /*** 🔴 STEP 10: DESTROY VULKAN INSTANCE LAST 🔴 ***/
        std::cout << "Destroying Vulkan instance..." << std::endl;
        if (instance) {
            instance.reset();
        }

        /*** 🔴 STEP 11: CLEANUP GLFW 🔴 ***/
        std::cout << "Destroying GLFW window..." << std::endl;
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }

        std::cout << "Terminating GLFW..." << std::endl;
        glfwTerminate();

        std::cout << "Cleanup complete! Vulkan shutdown successful." << std::endl;
    }


    // --------------------------------------------------------------------------
    // createVertexBuffer() creates the vertex buffer, allocates memory, maps and
    // copies the vertex data.
    // --------------------------------------------------------------------------
    void Application::createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // ✅ 1️⃣ Create a staging buffer in host-visible memory
        auto stagingBuffer = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,  // Staging buffer is used as transfer source
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        // ✅ 2️⃣ Map memory, copy data, then unmap
        void* mappedMemory;
        (void)device->mapMemory(*stagingBuffer.second, 0, bufferSize, {}, &mappedMemory);
        memcpy(mappedMemory, vertices.data(), static_cast<size_t>(bufferSize));
        device->unmapMemory(*stagingBuffer.second);

        // ✅ 3️⃣ Create vertex buffer in fast `DEVICE_LOCAL` memory
        auto vertexBufferPair = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        // ✅ 4️⃣ Copy from staging buffer to device-local buffer
        copyBuffer(*stagingBuffer.first, *vertexBufferPair.first, bufferSize);

        // ✅ 5️⃣ Cleanup staging buffer
        stagingBuffer.first.reset();
        stagingBuffer.second.reset();

        // ✅ 6️⃣ Assign vertex buffer
        vertexBuffer = std::move(vertexBufferPair.first);
        vertexBufferMemory = std::move(vertexBufferPair.second);

        std::cout << "Vertex buffer successfully uploaded: " << vertices.size() << " vertices." << std::endl;
    }


    // --------------------------------------------------------------------------
    // createIndexBuffer() creates an index buffer (similar to vertex buffer).
    // --------------------------------------------------------------------------
    inline void Application::createIndexBuffer() {

#ifdef _DEBUG
        std::cout << "===== Index Buffer Data =====" << std::endl;
        for (size_t i = 0; i < indices.size(); i += 3) {
            std::cout << "Triangle " << (i / 3) << ": "
                << indices[i] << ", "
                << indices[i + 1] << ", "
                << indices[i + 2] << std::endl;
        }
        std::cout << "=============================" << std::endl;
#endif

        if (indices.empty()) {
            throw std::runtime_error("Error: No index data available!");
        }

        vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        // 1️⃣ Create staging buffer (HOST_VISIBLE, HOST_COHERENT)
        auto stagingBuffer = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        // 2️⃣ Map memory, copy data, and unmap
        auto mapResult = device->mapMemory(*stagingBuffer.second, 0, bufferSize);
        if (mapResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to map staging buffer memory!");
        }

        memcpy(mapResult.value, indices.data(), static_cast<size_t>(bufferSize));
        device->unmapMemory(*stagingBuffer.second);

        // 3️⃣ Create index buffer (DEVICE_LOCAL)
        auto idxBuffer = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        if (!idxBuffer.first || !idxBuffer.second) {
            throw std::runtime_error("Failed to create index buffer!");
        }

        // 4️⃣ Copy staging buffer → index buffer
        copyBuffer(*stagingBuffer.first, *idxBuffer.first, bufferSize);

        // 5️⃣ Cleanup staging buffer
        stagingBuffer.first.reset();
        stagingBuffer.second.reset();

        // 6️⃣ Assign index buffer
        indexBuffer = std::move(idxBuffer.first);
        indexBufferMemory = std::move(idxBuffer.second);
    }


    // --------------------------------------------------------------------------
    // updateUniformBuffer() updates the uniform buffer for the given swapchain image.
    // Make sure its signature matches everywhere it's used.
    // --------------------------------------------------------------------------
    //void Application::updateUniformBuffer(uint32_t currentImage, const Camera::State& camera) {
    //    static auto startTime = std::chrono::high_resolution_clock::now();
    //    auto currentTime = std::chrono::high_resolution_clock::now();
    //    float time = std::chrono::duration<float>(currentTime - startTime).count();

    //    UniformBufferObject ubo{};
    //    // Rotate model around the Z axis; note we pass an axis vector as third argument.
    //    ubo.model = glm::rotate(glm::mat4(1.0f), cubeRotation * time, glm::vec3(0.0f, 0.0f, 1.0f));
    //    ubo.view = Camera::getViewMatrix(camera);
    //    // Set up perspective projection: fov, aspect, near, far.
    //    ubo.proj = glm::perspective(glm::radians(45.0f),
    //        swapChainExtent.width / static_cast<float>(swapChainExtent.height),
    //        0.1f, 10.0f);
    //    ubo.proj[1][1] *= -1; // Flip Y coordinate for Vulkan

    //    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    //}

    // --------------------------------------------------------------------------
    // createBuffer() helper: creates a buffer and allocates memory for it.
    // --------------------------------------------------------------------------
    //std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> Application::createBuffer(
    //    vk::DeviceSize size,
    //    vk::BufferUsageFlags usage,
    //    vk::MemoryPropertyFlags properties)
    //{
    //    if (!device) {
    //        throw std::runtime_error("Device not initialized!");
    //    }
    //    vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eExclusive);
    //    auto bufferResult = device->createBufferUnique(bufferInfo);
    //    if (bufferResult.result != vk::Result::eSuccess) {
    //        throw std::runtime_error("Failed to create buffer!");
    //    }
    //    vk::UniqueBuffer buffer = std::move(bufferResult.value);

    //    vk::MemoryRequirements memRequirements = device->getBufferMemoryRequirements(*buffer);
    //    uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    //    vk::MemoryAllocateInfo allocInfo(memRequirements.size, memoryTypeIndex);

    //    auto memoryResult = device->allocateMemoryUnique(allocInfo);
    //    if (memoryResult.result != vk::Result::eSuccess) {
    //        throw std::runtime_error("Failed to allocate buffer memory!");
    //    }
    //    vk::UniqueDeviceMemory bufferMemory = std::move(memoryResult.value);
    //    (void)device->bindBufferMemory(*buffer, *bufferMemory, 0);
    //    return { std::move(buffer), std::move(bufferMemory) };
    //}

    //// --------------------------------------------------------------------------
    //// findMemoryType() helper: returns a suitable memory type index.
    //// --------------------------------------------------------------------------
    //uint32_t Application::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    //    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    //    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    //        if ((typeFilter & (1 << i)) &&
    //            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
    //            return i;
    //        }
    //    }
    //    throw std::runtime_error("Failed to find suitable memory type!");
    //}

    //// --------------------------------------------------------------------------
    //// beginSingleTimeCommands() and endSingleTimeCommands() for short-lived command buffers.
    //// --------------------------------------------------------------------------
    //vk::UniqueCommandBuffer Application::beginSingleTimeCommands() {
    //    vk::CommandBufferAllocateInfo allocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1);
    //    auto result = device->allocateCommandBuffersUnique(allocInfo);
    //    if (result.value.empty()) {
    //        throw std::runtime_error("Failed to allocate command buffer!");
    //    }
    //    vk::UniqueCommandBuffer commandBuffer = std::move(result.value[0]);
    //    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    //    (void)commandBuffer->begin(beginInfo);
    //    return commandBuffer;
    //}

    //void Application::endSingleTimeCommands(vk::UniqueCommandBuffer& commandBuffer) {
    //    (void)commandBuffer->end();
    //    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &*commandBuffer);
    //    (void)graphicsQueue.submit(submitInfo, nullptr);
    //    (void)graphicsQueue.waitIdle();
    //}

    //// --------------------------------------------------------------------------
    //// copyBuffer() copies data from srcBuffer to dstBuffer.
    //// --------------------------------------------------------------------------
    //void Application::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
    //    vk::UniqueCommandBuffer commandBuffer = beginSingleTimeCommands();
    //    vk::BufferCopy copyRegion(0, 0, size);
    //    commandBuffer->copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    //    endSingleTimeCommands(commandBuffer);
    //}

    // --------------------------------------------------------------------------
    // Other functions such as createSurface(), createDepthResources(), createSwapChain(), etc.
    // (They remain similar to your original implementation, with any minor fixes as needed.)
    // --------------------------------------------------------------------------

} // namespace VulkanCube

//#define VULKAN_CUBE_MAIN
#ifdef VULKAN_CUBE_MAIN

int main() {
    VulkanCube::Application app;
    try {
        app.initWindow();
        app.initVulkan(); // Fully initializes Vulkan and creates all resources.
        app.mainLoop();
        app.cleanup();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
#endif
