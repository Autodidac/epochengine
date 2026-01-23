module;

#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>

#ifndef APIENTRY
#define APIENTRY
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS
#define ENABLE_VALIDATION_LAYERS 
#include <vulkan/vulkan.hpp>

// GLM mathematics
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// stb image loader
#include "stb/stb_image.h"

// Camera controls
//#include "vulkan_camera.hpp"

export module acontext.vulkan.context;

import acontext.vulkan.camera;

namespace VulkanCube {

    // Debug callback for validation layers
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* /*pUserData*/)
    {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "[Validation] " << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }

#ifdef NDEBUG
    inline const bool enableValidationLayers = false;
#else
    inline const bool enableValidationLayers = true;
#endif

    // Validation layers, device extensions
    inline const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    inline const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    class Application {

    public:
        void run() {
            initWindow();
            initVulkan();

            std::cout << "Vertex struct size: " << sizeof(Vertex) << std::endl;
            std::cout << "Position offset: " << offsetof(Vertex, pos) << std::endl;
            std::cout << "Normal offset: " << offsetof(Vertex, normal) << std::endl;
            std::cout << "TexCoord offset: " << offsetof(Vertex, texCoord) << std::endl;


            mainLoop();
            cleanup();
        }

        // Public Function declarations
        void initWindow();
        void initVulkan();
        void mainLoop();
        void cleanup();
        void createVertexBuffer();
        void createIndexBuffer();

        // Window
        GLFWwindow* window = nullptr;
        vk::UniqueDevice device;
        vk::PhysicalDevice physicalDevice;
        void drawFrame();

        // fft ocean stuff
        vk::CommandBuffer getCurrentCommandBuffer() const {
            return *commandBuffers[currentFrame];
        }



    private:

        std::vector<vk::UniqueCommandBuffer> commandBuffers;
        // Vulkan core objects
        vk::UniqueInstance instance;
        // raw debug messenger to fix C2679
        vk::DebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        vk::UniqueSurfaceKHR surface;
        vk::Queue graphicsQueue;
        vk::Queue presentQueue;
        QueueFamilyIndices queueFamilyIndices;

        // Swap chain
        vk::UniqueSwapchainKHR swapChain;
        std::vector<vk::Image> swapChainImages;
        vk::Format swapChainImageFormat;
        vk::Extent2D swapChainExtent;
        std::vector<vk::UniqueImageView> swapChainImageViews;

        // Render pass and pipeline
        vk::UniqueRenderPass renderPass;
        vk::UniqueDescriptorSetLayout descriptorSetLayout;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline graphicsPipeline;

        // Framebuffers
        std::vector<vk::UniqueFramebuffer> framebuffers;
        // Command pool and command buffers
        vk::UniqueCommandPool commandPool;

        // Buffers and images
        vk::UniqueBuffer vertexBuffer;
        vk::UniqueDeviceMemory vertexBufferMemory;

        // Index Buffer
        //std::vector<uint16_t> indices;               // The index data
        vk::UniqueBuffer indexBuffer;                // The GPU buffer for indices
        vk::UniqueDeviceMemory indexBufferMemory;    // The memory bound to the index buffer

        std::vector<vk::UniqueBuffer> uniformBuffers;
        std::vector<vk::UniqueDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;
        vk::UniqueImage textureImage;
        vk::UniqueDeviceMemory textureImageMemory;
        vk::UniqueImageView textureImageView;
        vk::UniqueSampler textureSampler;

        // Descriptor sets
        vk::UniqueDescriptorPool descriptorPool;
        std::vector<vk::UniqueDescriptorSet> descriptorSets;

        // Synchronization
        std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
        std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
        std::vector<vk::UniqueFence> inFlightFences;
        size_t currentFrame = 0;
        bool framebufferResized = false;

        // depth resources
        vk::UniqueImage depthImage;
        vk::UniqueDeviceMemory depthImageMemory;
        vk::UniqueImageView depthImageView;

        // Camera
        inline static Camera::State cam = Camera::create(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            -90.0f, 0.0f
        );
        float lastX = 400.0f;
        float lastY = 300.0f;
        bool firstMouse = true;
        float cubeRotation = 1.0f;

        // Function declarations
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
        void processMouseInput(double xpos, double ypos);
        void updateCamera(float deltaTime);

        void createInstance();
        bool checkValidationLayerSupport();
        std::vector<const char*> getRequiredExtensions();
        //void setupDebugMessenger();
        //void cleanupDebugMessenger();
        void createSurface();
        vk::PhysicalDevice pickPhysicalDevice();
        bool isDeviceSuitable(vk::PhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
        bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
        void createLogicalDevice();
        void createCommandPool();

        SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);
        vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
        vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& presentModes);
        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
        void createSwapChain();
        vk::UniqueImageView createImageViewUnique(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
        void createImageViews();
        void createRenderPass();
        void createDescriptorSetLayout();
        void createGraphicsPipeline();

        void createDepthResources();
        vk::Format findDepthFormat();
        vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
        void createFramebuffers();

        // We keep only vertex buffer methods
        std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        vk::UniqueCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(vk::UniqueCommandBuffer& commandBuffer);
        void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

        void createUniformBuffers();
        void updateUniformBuffer(uint32_t currentImage, const Camera::State& camera);

        void createDescriptorPool();
        void createDescriptorSets();

        void createTextureImage();
        void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
        void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
        void createTextureImageView();
        void createTextureSampler();

        void createCommandBuffers();
        void createSyncObjects();
        void recreateSwapChain();
        void cleanupSwapChain();

        static std::vector<char> readFile(const std::string& filename);

        // 24 vert texturing
        struct Vertex {
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec2 texCoord;

            static vk::VertexInputBindingDescription getBindingDescription() {
                return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
            }

            static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
                return { {
                    {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},       // Position
                    {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},    // Normal
                    {2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)}      // Texture Coordinate
                } };
            }

        };

        // 24 vertices
        const std::vector<Vertex> vertices = {
            // Front face (z = 0.5)
            {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0}},
            {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0}},
            {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1}},
            {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1}},
            // Back face (z = -0.5)
            {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 0}},
            {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 0}},
            {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1}},
            {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0, 1}},
            // Left face (x = -0.5)
            {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 0}},
            {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 0}},
            {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 1}},
            {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0, 1}},
            // Right face (x = 0.5)
            {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0, 0}},
            {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 0}},
            {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1, 1}},
            {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0, 1}},
            // Top face (y = 0.5)
            {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0, 0}},
            {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 0}},
            {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1}},
            {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0, 1}},
            // Bottom face (y = -0.5)
            {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 0}},
            {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 0}},
            {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 1}},
            {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 1}},
        };

        const std::vector<uint16_t> indices = {
            0, 1, 2,  0, 2, 3,       // Front
            4, 5, 6,  4, 6, 7,       // Back
            8, 9, 10,  8, 10, 11,    // Left
            12, 13, 14,  12, 14, 15, // Right
            16, 17, 18,  16, 18, 19, // Top
            20, 21, 22,  20, 22, 23  // Bottom
        };

    };

} // namespace VulkanCube

