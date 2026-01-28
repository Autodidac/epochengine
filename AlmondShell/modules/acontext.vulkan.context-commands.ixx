// ============================================================================
// modules/acontext.vulkan.context-commands.ixx
// Partition implementation: acontext.vulkan.context:commands
// Command buffers + sync + per-frame submit/present.
// ============================================================================

module;

#include <include/acontext.vulkan.hpp>
// Include Vulkan-Hpp after config.
#include <vulkan/vulkan.hpp>

export module acontext.vulkan.context:commands;

import :shared_vk;
import aengine.core.context;

import <array>;
import <cstdint>;
import <limits>;
import <stdexcept>;

namespace almondnamespace::vulkancontext
{
    inline constexpr std::size_t kMaxFramesInFlight = 2;

    void Application::createCommandBuffers()
    {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = *commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = static_cast<std::uint32_t>(swapChainImages.size());

        auto [allocRes, bufs] = device->allocateCommandBuffersUnique(allocInfo);
        if (allocRes != vk::Result::eSuccess)
            throw std::runtime_error("[Vulkan] allocateCommandBuffersUnique failed.");
        commandBuffers = std::move(bufs);

        std::array<vk::ClearValue, 2> clearValues{};
        const auto clearColor = almondnamespace::core::clear_color_for_context(
            almondnamespace::core::ContextType::Vulkan);
        clearValues[0].setColor(
            vk::ClearColorValue{ std::array<float, 4>{ clearColor[0], clearColor[1], clearColor[2], clearColor[3] } });
        clearValues[1].setDepthStencil(vk::ClearDepthStencilValue{ 1.0f, 0 });

        // You MUST have this set when you create/fill the index buffer.
        const std::uint32_t safeIndexCount = indexCount;
        if (safeIndexCount == 0)
            throw std::runtime_error("[Vulkan] indexCount is 0. Set Application::indexCount when creating the index buffer.");

        for (std::size_t i = 0; i < commandBuffers.size(); ++i)
        {
            vk::CommandBufferBeginInfo beginInfo{};
            if (commandBuffers[i]->begin(beginInfo) != vk::Result::eSuccess)
                throw std::runtime_error("[Vulkan] CommandBuffer::begin failed.");

            vk::RenderPassBeginInfo renderPassInfo{};
            renderPassInfo.renderPass = *renderPass;
            renderPassInfo.framebuffer = *framebuffers[i];
            renderPassInfo.renderArea = vk::Rect2D{ vk::Offset2D{0, 0}, swapChainExtent };
            renderPassInfo.clearValueCount = static_cast<std::uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            commandBuffers[i]->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

            commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

            const vk::Buffer vb[] = { *vertexBuffer };
            const vk::DeviceSize offsets[] = { 0 };
            commandBuffers[i]->bindVertexBuffers(0, 1, vb, offsets);

            commandBuffers[i]->bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                *pipelineLayout,
                0,
                1,
                &*descriptorSets[i],
                0,
                nullptr
            );

            commandBuffers[i]->bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);

            // Dispatcher-aware overload (matches the signature MSVC showed you).
            commandBuffers[i]->drawIndexed(
                safeIndexCount,
                1u,
                0u,
                0,
                0u,
                VULKAN_HPP_DEFAULT_DISPATCHER
            );

            commandBuffers[i]->endRenderPass();

            if (commandBuffers[i]->end() != vk::Result::eSuccess)
                throw std::runtime_error("[Vulkan] CommandBuffer::end failed.");
        }
    }

    void Application::createSyncObjects()
    {
        imageAvailableSemaphores.resize(kMaxFramesInFlight);
        renderFinishedSemaphores.resize(kMaxFramesInFlight);
        inFlightFences.resize(kMaxFramesInFlight);

        vk::SemaphoreCreateInfo semInfo{};
        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

        for (std::size_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            {
                auto [r, sem] = device->createSemaphoreUnique(semInfo);
                if (r != vk::Result::eSuccess)
                    throw std::runtime_error("[Vulkan] createSemaphoreUnique(imageAvailable) failed.");
                imageAvailableSemaphores[i] = std::move(sem);
            }
            {
                auto [r, sem] = device->createSemaphoreUnique(semInfo);
                if (r != vk::Result::eSuccess)
                    throw std::runtime_error("[Vulkan] createSemaphoreUnique(renderFinished) failed.");
                renderFinishedSemaphores[i] = std::move(sem);
            }
            {
                auto [r, f] = device->createFenceUnique(fenceInfo);
                if (r != vk::Result::eSuccess)
                    throw std::runtime_error("[Vulkan] createFenceUnique failed.");
                inFlightFences[i] = std::move(f);
            }
        }
    }

    void Application::drawFrame()
    {
        const auto timeout = (std::numeric_limits<std::uint64_t>::max)();

        // Wait for CPU/GPU sync for this frame.
        {
            vk::Fence f = *inFlightFences[currentFrame];
            const vk::Result r = device->waitForFences(1, &f, VK_TRUE, timeout);
            if (r != vk::Result::eSuccess)
                throw std::runtime_error("[Vulkan] waitForFences failed.");
        }

        std::uint32_t imageIndex = 0;
        vk::Result acquireRes = device->acquireNextImageKHR(
            *swapChain,
            timeout,
            *imageAvailableSemaphores[currentFrame],
            vk::Fence{},
            &imageIndex
        );

        if (acquireRes == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapChain();
            return;
        }
        if (acquireRes != vk::Result::eSuccess && acquireRes != vk::Result::eSuboptimalKHR)
            throw std::runtime_error("[Vulkan] Failed to acquire swap chain image.");

        if (framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }

        // Reset fence for this frame.
        {
            vk::Fence f = *inFlightFences[currentFrame];
            const vk::Result r = device->resetFences(1, &f);
            if (r != vk::Result::eSuccess)
                throw std::runtime_error("[Vulkan] resetFences failed.");
        }

        updateUniformBuffer(imageIndex, cam);

        const vk::Semaphore waitSems[] = { *imageAvailableSemaphores[currentFrame] };
        const vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        const vk::CommandBuffer submitCmds[] = { *commandBuffers[imageIndex] };
        const vk::Semaphore signalSems[] = { *renderFinishedSemaphores[currentFrame] };

        vk::SubmitInfo submitInfo{};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSems;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = submitCmds;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSems;

        {
            const vk::Result r = graphicsQueue.submit(1, &submitInfo, *inFlightFences[currentFrame]);
            if (r != vk::Result::eSuccess)
                throw std::runtime_error("[Vulkan] graphicsQueue.submit failed.");
        }

        const vk::SwapchainKHR scs[] = { *swapChain };

        vk::PresentInfoKHR presentInfo{};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSems;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = scs;
        presentInfo.pImageIndices = &imageIndex;

        vk::Result presentRes = presentQueue.presentKHR(presentInfo);

        if (presentRes == vk::Result::eErrorOutOfDateKHR || presentRes == vk::Result::eSuboptimalKHR)
        {
            recreateSwapChain();
        }
        else if (presentRes != vk::Result::eSuccess)
        {
            throw std::runtime_error("[Vulkan] presentKHR failed.");
        }

        currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
    }
} // namespace almondnamespace::vulkancontext
