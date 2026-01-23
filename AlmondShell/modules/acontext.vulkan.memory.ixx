module;



export module acontext.vulkan.memory;

import acontext.vulkan.context;

namespace VulkanCube {

    uint32_t Application::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type!");
    }

    // create buffer
    inline std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> Application::createBuffer(
        vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties
    ) {
        // 1️⃣ Create Buffer
        vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eExclusive);
        auto bufferResult = device->createBufferUnique(bufferInfo);
        if (bufferResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create buffer!");
        }
        vk::UniqueBuffer buffer = std::move(bufferResult.value);

        // 2️⃣ Get Memory Requirements (CORRECTLY)
        vk::MemoryRequirements memRequirements = device->getBufferMemoryRequirements(*buffer);

        // 3️⃣ Allocate Memory
        vk::MemoryAllocateInfo allocInfo(
            memRequirements.size,
            findMemoryType(memRequirements.memoryTypeBits, properties)
        );

        auto memoryResult = device->allocateMemoryUnique(allocInfo);
        if (memoryResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }
        vk::UniqueDeviceMemory bufferMemory = std::move(memoryResult.value);

        // 4️⃣ Bind Memory
        (void)device->bindBufferMemory(*buffer, *bufferMemory, 0);

        return { std::move(buffer), std::move(bufferMemory) };
    }

    inline vk::UniqueCommandBuffer Application::beginSingleTimeCommands() {
        vk::CommandBufferAllocateInfo allocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1);
        auto allocResult = device->allocateCommandBuffersUnique(allocInfo);
        if (allocResult.value.empty()) {
            throw std::runtime_error("Failed to allocate command buffer!\n");
        }
        vk::UniqueCommandBuffer commandBuffer = std::move(allocResult.value[0]);
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        (void)commandBuffer->begin(beginInfo);
        return commandBuffer;
    }

    inline void Application::endSingleTimeCommands(vk::UniqueCommandBuffer& commandBuffer) {
        (void)commandBuffer->end();
        vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &*commandBuffer);
        (void)graphicsQueue.submit(submitInfo, VK_NULL_HANDLE);
        (void)graphicsQueue.waitIdle();
        // UniqueCommandBuffer will be freed automatically when it goes out of scope.
    }

    inline void Application::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
        std::cout << "Copying " << size << " bytes from staging buffer to GPU vertex buffer.\n";
        vk::UniqueCommandBuffer commandBuffer = beginSingleTimeCommands();
        vk::BufferCopy copyRegion(0, 0, size);
        commandBuffer->copyBuffer(srcBuffer, dstBuffer, copyRegion);
        endSingleTimeCommands(commandBuffer);
    }

} // namespace VulkanCube
