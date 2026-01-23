module;



export module acontext.vulkan.commands;

import acontext.vulkan.context;

namespace VulkanCube {

    const int MAX_FRAMES_IN_FLIGHT = 2;
    //inline uint32_t imageIndex = 0;

    inline void Application::createCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo{
            *commandPool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(swapChainImages.size())
        };

        commandBuffers = device->allocateCommandBuffersUnique(allocInfo).value;

        std::array<vk::ClearValue, 2> clearValues{};
        clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.5f, 0.5f, 1.0f});
        clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

        for (size_t i = 0; i < commandBuffers.size(); ++i) {
            vk::CommandBufferBeginInfo beginInfo{};
            (void)commandBuffers[i]->begin(beginInfo);

            vk::RenderPassBeginInfo renderPassInfo(
                *renderPass, *framebuffers[i],
                vk::Rect2D{ {0, 0}, swapChainExtent },
                (uint32_t)clearValues.size(), clearValues.data()
            );

            commandBuffers[i]->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

            assert(*graphicsPipeline);
            commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

            // Proper vertex buffer binding
            vk::Buffer vertexBuffers[] = { *vertexBuffer };
            vk::DeviceSize offsets[] = { 0 };
            commandBuffers[i]->bindVertexBuffers(0, 1, vertexBuffers, offsets);
            // commandBuffers[i]->bindVertexBuffers(0, 1, &vertices, offsets);

             // Descriptor Sets Binding (perfect Vulkan-HPP usage)
            commandBuffers[i]->bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                *pipelineLayout, 0, *descriptorSets[i], {}
            );

            // Correct draw call (indexed or non-indexed)
            commandBuffers[i]->bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
            commandBuffers[i]->drawIndexed(
                static_cast<uint32_t>(indices.size()),  // ✅ Use indices, not vertex count
                1, 0, 0, 0
            );


            commandBuffers[i]->endRenderPass();
            (void)commandBuffers[i]->end();
        }
    }


    inline void Application::createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        vk::SemaphoreCreateInfo semInfo{};
        vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            imageAvailableSemaphores[i] = device->createSemaphoreUnique(semInfo).value;
            renderFinishedSemaphores[i] = device->createSemaphoreUnique(semInfo).value;
            inFlightFences[i] = device->createFenceUnique(fenceInfo).value;
        }
        // std::cout << "Sync objects created, imageAvailableSemaphores.size(): " << imageAvailableSemaphores.size() << std::endl;
    }


    inline void Application::drawFrame() {
        // Make sure synchronization objects have been created!
        // std::cout << "currentFrame: " << currentFrame << ", imageAvailableSemaphores.size(): " << imageAvailableSemaphores.size() << "\n";
        // ✅ Wait for the previous frame to finish before acquiring a new image
        (void)device->waitForFences(1, &*inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        // Acquire image using a local imageIndex variable:
        uint32_t imageIndex = 0;
        auto result = device->acquireNextImageKHR(
            *swapChain,
            UINT64_MAX,
            *imageAvailableSemaphores[currentFrame],
            nullptr,   // pass nullptr instead of a fence here
            &imageIndex
        );

        // std::cout << "Acquired imageIndex: " << imageIndex << ", commandBuffers.size(): " << commandBuffers.size() << "\n";

        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapChain();
            return;
        }
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        if (framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }

        // Reset fence for this frame (no need to resize sync objects here)
        (void)device->resetFences(1, &*inFlightFences[currentFrame]);

        // Update UBO with the acquired image index
        updateUniformBuffer(imageIndex, cam);

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo submitInfo(
            1, &*imageAvailableSemaphores[currentFrame], &waitStage,
            1, &*commandBuffers[imageIndex],  // Make sure commandBuffers is sized correctly!
            1, &*renderFinishedSemaphores[currentFrame]
        );

        (void)graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfo(
            1, &*renderFinishedSemaphores[currentFrame],
            1, &*swapChain,
            &imageIndex
        );

        result = presentQueue.presentKHR(presentInfo);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
            recreateSwapChain();
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    inline void Application::recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        (void)device->waitIdle();
        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        // re-use descriptorSetLayout
        createGraphicsPipeline();
        // if you want depth:
        createDepthResources();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
    }

    inline void Application::cleanupSwapChain() {
        // if you have depth, handle it here
        depthImageView.reset();
        depthImageMemory.reset();
        depthImage.reset();

        for (auto& framebuffer : framebuffers) {
            framebuffer.reset();
        }
        framebuffers.clear();

        for (auto& imageView : swapChainImageViews) {
            imageView.reset();
        }
        swapChainImageViews.clear();

        swapChain.reset();

        // unmap & reset uniform buffers
        for (size_t i = 0; i < uniformBuffersMemory.size(); ++i) {
            if (uniformBuffersMemory[i]) {
                device->unmapMemory(*uniformBuffersMemory[i]);
            }
            uniformBuffersMemory[i].reset();
            uniformBuffers[i].reset();
        }
        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();

        descriptorSets.clear();
        descriptorPool.reset();
        commandBuffers.clear();
    }
} // namespace VulkanCube
