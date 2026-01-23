module;



export module acontext.vulkan.shader.pipeline;

import acontext.vulkan.context;

namespace VulkanCube {

    inline void Application::createRenderPass() {
        vk::AttachmentDescription colorAttachment({}, swapChainImageFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
        );

        vk::AttachmentDescription depthAttachment(
            {}, findDepthFormat(), vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
        );

        std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::SubpassDescription subpass(
            {}, vk::PipelineBindPoint::eGraphics,
            0, nullptr,                 // input attachments
            1, &colorAttachmentRef,     // color attachments
            nullptr,                    // resolve attachments <- explicitly nullptr if not used!
            &depthAttachmentRef,        // depth attachment
            0, nullptr                  // preserve attachments
        );

        vk::SubpassDependency dependency(
            {}, {},
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
            {},
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::DependencyFlagBits::eByRegion // Add this flag
        );

        vk::RenderPassCreateInfo renderPassInfo({}, attachments, subpass, dependency);
        renderPass = device->createRenderPassUnique(renderPassInfo).value;
    }


    inline void Application::createDescriptorSetLayout() {
        vk::DescriptorSetLayoutBinding uboLayoutBinding(
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex
        );
        vk::DescriptorSetLayoutBinding samplerLayoutBinding(
            1, vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment
        );
        std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, static_cast<uint32_t>(bindings.size()), bindings.data());
        auto result = device->createDescriptorSetLayoutUnique(layoutInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
        descriptorSetLayout = std::move(result.value);
    }

    inline std::vector<char> Application::readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    inline void Application::createGraphicsPipeline() {
        // Load shaders
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        auto vertShaderModule = device->createShaderModuleUnique({
            {}, vertShaderCode.size(), reinterpret_cast<const uint32_t*>(vertShaderCode.data())
            }).value;

        auto fragShaderModule = device->createShaderModuleUnique({
            {}, fragShaderCode.size(), reinterpret_cast<const uint32_t*>(fragShaderCode.data())
            }).value;

        vk::PipelineShaderStageCreateInfo shaderStages[] = {
            {{}, vk::ShaderStageFlagBits::eVertex,   *vertShaderModule, "main"},
            {{}, vk::ShaderStageFlagBits::eFragment, *fragShaderModule, "main"}
        };

        // Persistently store these!
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
            {}, 1, &bindingDescription,
            static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data()
        );

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList);

        vk::Viewport viewport{ 0.f, 0.f, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.f, 1.f };
        vk::Rect2D scissor({ 0, 0 }, swapChainExtent);
        vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

        vk::PipelineRasterizationStateCreateInfo rasterizer({}, VK_FALSE, VK_FALSE,
            vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise);
        rasterizer.lineWidth = 1.0f;

        vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1);
        vk::PipelineDepthStencilStateCreateInfo depthStencil({}, VK_TRUE, VK_TRUE, vk::CompareOp::eLess);

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_FALSE;

        vk::PipelineColorBlendStateCreateInfo colorBlending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

        // Pipeline layout creation
        assert(descriptorSetLayout && *descriptorSetLayout);
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 1, &*descriptorSetLayout);
        pipelineLayout = device->createPipelineLayoutUnique(pipelineLayoutInfo).value;

        // Graphics pipeline creation
        vk::GraphicsPipelineCreateInfo pipelineInfo(
            {}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr,
            &viewportState, &rasterizer, &multisampling, &depthStencil,
            &colorBlending, nullptr, *pipelineLayout, *renderPass
        );

        graphicsPipeline = device->createGraphicsPipelineUnique({}, pipelineInfo).value;

        // explicit verification
        assert(*graphicsPipeline);
    }

} // namespace VulkanCube
