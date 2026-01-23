module;



export module acontext.vulkan.descriptors;

import acontext.vulkan.context;

namespace VulkanCube {

    inline void Application::createDescriptorPool() {
        std::array<vk::DescriptorPoolSize, 2> poolSizes = { {
            {vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(swapChainImages.size())},
            {vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(swapChainImages.size())}
        } };

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;  // ✅ Add this
        poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        descriptorPool = device->createDescriptorPoolUnique(poolInfo).value;
    }


    inline void Application::createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), *descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, static_cast<uint32_t>(layouts.size()), layouts.data());
        auto result = device->allocateDescriptorSetsUnique(allocInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }
        descriptorSets = std::move(result.value);
        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            vk::DescriptorBufferInfo bufferInfo(*uniformBuffers[i], 0, sizeof(UniformBufferObject));
            vk::DescriptorImageInfo imageInfo(*textureSampler, *textureImageView, vk::ImageLayout::eShaderReadOnlyOptimal);
            std::array<vk::WriteDescriptorSet, 2> descriptorWrites = { {
                vk::WriteDescriptorSet(*descriptorSets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo),
                vk::WriteDescriptorSet(*descriptorSets[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo, nullptr)
            } };

            device->updateDescriptorSets(descriptorWrites, nullptr);
        }

    }

    inline void Application::createUniformBuffers() {
        uniformBuffers.resize(swapChainImages.size());
        uniformBuffersMemory.resize(swapChainImages.size());
        uniformBuffersMapped.resize(swapChainImages.size());
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            vk::BufferCreateInfo bufferInfo({}, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer);
            uniformBuffers[i] = device->createBufferUnique(bufferInfo).value;
            vk::MemoryRequirements memReq = device->getBufferMemoryRequirements(*uniformBuffers[i]);
            uint32_t memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            vk::MemoryAllocateInfo allocInfo(memReq.size, memoryTypeIndex);
            uniformBuffersMemory[i] = device->allocateMemoryUnique(allocInfo).value;
            (void)device->bindBufferMemory(*uniformBuffers[i], *uniformBuffersMemory[i], 0);
            // Keep memory mapped for the life of the uniform buffer (host-coherent)
            auto mapResult = device->mapMemory(*uniformBuffersMemory[i], 0, bufferSize);
            if (mapResult.result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to map uniform buffer!");
            }
            uniformBuffersMapped[i] = mapResult.value;

        }
    }

    inline void Application::updateUniformBuffer(uint32_t currentImage, const Camera::State& camera)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        // Use the passed in value to calculate the model transformation.
        ubo.model = glm::rotate(glm::mat4(1.0f), cubeRotation * time, glm::vec3(0.0f, 0.0f, 1.0f));
        // Use the provided camera state to get the view matrix.
        ubo.view = Camera::getViewMatrix(camera);
        // Set up perspective projection (flip Y for Vulkan clip space)
        glm::mat4 proj = glm::perspective(glm::radians(45.0f),
            swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
        proj[1][1] *= -1;
        ubo.proj = proj;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

} // namespace VulkanCube
