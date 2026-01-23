module;

#include <vulkan/vulkan.h>

export module acontext.vulkan.dispatcher;

import acontext.vulkan.platform.loader;     //import the loader

namespace epoch::vulkan {

    // Global function to get vkGetInstanceProcAddr from the Vulkan Loader.
    inline auto getInstanceProcAddr() noexcept -> PFN_vkGetInstanceProcAddr {
        static PFN_vkGetInstanceProcAddr fp = nullptr;
        if (!fp) {
            void* lib = LoadLibrary();
            if (lib) {
                fp = reinterpret_cast<PFN_vkGetInstanceProcAddr>(LoadFunction(lib, "vkGetInstanceProcAddr"));
            }
        }
        return fp;
    }

    // Global function to get vkGetDeviceProcAddr from the Vulkan Loader.
    inline auto getDeviceProcAddr() noexcept -> PFN_vkGetDeviceProcAddr {
        static PFN_vkGetDeviceProcAddr fp = nullptr;
        if (!fp) {
            void* lib = LoadLibrary();
            if (lib) {
                fp = reinterpret_cast<PFN_vkGetDeviceProcAddr>(LoadFunction(lib, "vkGetDeviceProcAddr"));
            }
        }
        return fp;
    }

    // Loader-based retrieval of instance-level functions.
    inline auto GetInstanceFunction(VkInstance instance, const char* funcName) noexcept -> PFN_vkVoidFunction {
        auto fp = getInstanceProcAddr();
        return fp ? fp(instance, funcName) : nullptr;
    }

    // Loader-based retrieval of device-level functions.
    inline auto GetDeviceFunction(VkDevice device, const char* funcName) noexcept -> PFN_vkVoidFunction {
        auto fp = getDeviceProcAddr();
        return fp ? fp(device, funcName) : nullptr;
    }

    // Dynamic dispatch table for instance-level functions.
    struct InstanceDispatchTable {
        PFN_vkDestroyInstance vkDestroyInstance{};
        PFN_vkCreateDevice    vkCreateDevice{};
        // Additional instance-level functions can be added here.
    };

    inline auto createInstanceDispatchTable(VkInstance instance) -> InstanceDispatchTable {
        InstanceDispatchTable table{};
        table.vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(GetInstanceFunction(instance, "vkDestroyInstance"));
        table.vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(GetInstanceFunction(instance, "vkCreateDevice"));
        // Load more functions as required.
        return table;
    }

    // Dynamic dispatch table for device-level functions.
    struct DeviceDispatchTable {
        PFN_vkDestroyDevice vkDestroyDevice{};
        PFN_vkQueueSubmit   vkQueueSubmit{};
        // Additional device-level functions can be added here.
    };

    inline auto createDeviceDispatchTable(VkDevice device) -> DeviceDispatchTable {
        DeviceDispatchTable table{};
        table.vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(GetDeviceFunction(device, "vkDestroyDevice"));
        table.vkQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(GetDeviceFunction(device, "vkQueueSubmit"));
        // Load more functions as required.
        return table;
    }

}
