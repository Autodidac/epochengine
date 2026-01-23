module;

// VulkanInstance.hpp - Header-only Vulkan Instance creation and management
#include <vulkan/vulkan.h>

export module acontext.vulkan.platform.instance;

import acontext.vulkan.platform.dispatcher;

namespace epoch::vulkan {

    // Creates a Vulkan instance using loader-based dispatch (global function retrieval)
    inline auto createInstance(const VkInstanceCreateInfo& createInfo) -> VkInstance {
        VkInstance instance = VK_NULL_HANDLE;
        auto fp = getInstanceProcAddr();
        if (!fp) return VK_NULL_HANDLE;
        // Retrieve vkCreateInstance using the global loader (with a null instance)
        auto vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(fp(nullptr, "vkCreateInstance"));
        if (vkCreateInstance && vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS) {
            return instance;
        }
        return VK_NULL_HANDLE;
    }

    // Destroy a Vulkan instance using the dispatch table.
    inline void destroyInstance(VkInstance instance, const InstanceDispatchTable& table) {
        if (table.vkDestroyInstance && instance) {
            table.vkDestroyInstance(instance, nullptr);
        }
    }

}
